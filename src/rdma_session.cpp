/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_SESSION_CPP
************************************************ */

#include <string.h>
#include <stdio.h>
#include <functional>
#include <map>

#include "rdma_session.h"
#include "rdma_buffer.h"
#include "rdma_channel.h"
#include "rdma_endpoint.h"
#include "rdma_pre.h"
#include "../utils/ThreadPool/src/ThreadPool.h"

#define MSG_SIZE 20

RDMA_Session::RDMA_Session(char* dev_name)
{
    dev_name_ = dev_name;

    // init all of the resources, so cleanup will be easy
    if (open_ib_device())
    {
        //goto cleanup;
        return;
    }

    // allocate Protection Domain
    pd_ = ibv_alloc_pd(context_);
    if (!pd_)
    {
        log_error("Failed to allocate protection domain");
        //goto cleanup;
        return;
    }

    // Create completion channel
    event_channel_ = ibv_create_comp_channel(context_);
    if (!event_channel_)
    {
        log_error("Failed to create completion endpoint");
        //goto cleanup;
        return;
    }

    cq_ = ibv_create_cq(context_, CQ_SIZE, NULL, event_channel_, 0);
    if (!cq_)
    {
        log_error(make_string("Failed to create CQ with %u entries", CQ_SIZE));
        //goto cleanup;
        return;
    }

    if (ibv_req_notify_cq(cq_, 0))
    {
        log_error("Countn't request CQ notification");
        //goto cleanup;
        return;
    }

    // Use a new thread to do the CQ processing
    process_thread_.reset(new std::thread(std::bind(&RDMA_Session::session_processCQ, this)));

    pool_ = new ThreadPool(DEFAULT_POOL_THREADS);
    if (pool_)
    {
        log_info("ThreadPool Created");
    } else
    {
        log_error("Failed to create ThreadPool");
        return;
    }

    log_info("RDMA_Session Created");
}

RDMA_Session::~RDMA_Session()
{
    pool_->wait();
    delete pool_;

    stop_process();

    for (auto i:endpoint_list_)
        delete i;
    
    process_thread_.reset();
    
    if (ibv_destroy_cq(cq_))
    {
        log_error("Failed to destroy CQ");
    }

    if (ibv_destroy_comp_channel(event_channel_))
    {
        log_error("Failed to destroy completion channel");
    }

    if (ibv_dealloc_pd(pd_))
    {
        log_error("Failed to deallocate PD");
    }

    log_info("RDMA_Session Deleted");
}

int RDMA_Session::open_ib_device()
{
    int i, num_devices;
    struct ibv_device *ib_dev = NULL;
    struct ibv_device **dev_list;

    log_info("Starting Resources Initialization");
    log_info("Searching for IB devices in host");

    // get device names in the system
    dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list)
    {
        log_error("Failed to get IB devices list");
        return 1;
    }

    // if there isn't any IB device in host
    if (!num_devices)
    {
        log_error(make_string("IB device not found"));
        return 1;
    } else log_info(make_string("Found %d device(s)", num_devices));

    if (!dev_name_)
    {
        ib_dev = *dev_list;
        if (!ib_dev)
        {
            log_error("No IB devices found");
            return 1;
        } log_info(make_string("Choose the first IB device"));
    } else 
    {
        // search for the specific device we want to work with
        for (i = 0; i < num_devices; i ++)
        {
            if (!strcmp(ibv_get_device_name(dev_list[i]), dev_name_))
            {
                ib_dev = dev_list[i];
                break;
            }
        }
        // if the device wasn't found in host
        if (!ib_dev)
        {
            log_error(make_string("IB device %s wasn't found", dev_name_));
            return 1;
        }
    }

    // get device handle
    context_ = ibv_open_device(ib_dev);
    if (!context_)
    {
        log_error(make_string("Failed to open device %s", dev_name_));
        return 1;
    }

    return 0;
}

void RDMA_Session::stop_process()
{
    process_thread_->join();
}

RDMA_Endpoint* RDMA_Session::new_endpoint(RDMA_Pre* pre)
{
    RDMA_Endpoint* new_endpoint = new RDMA_Endpoint(this, pre->config.ib_port);
    endpoint_list_.push_back(new_endpoint);
    new_endpoint->connect(pre->exchange_qp_data(new_endpoint->get_local_con_data()));

    return new_endpoint;
}

RDMA_Endpoint* RDMA_Session::ptp_connect(RDMA_Pre* pre)
{
    pre->ptp_connect();

    return new_endpoint(pre);
}

void RDMA_Session::daemon_connect(RDMA_Pre* pre)
{
    pre_ = pre;
    std::function<void()> func = std::bind(&RDMA_Session::new_endpoint, this, pre);
    pre->daemon_connect(func);
}

// -----------------------------------

void RDMA_Session::session_processCQ()
{
    enum Session_status
    {
        WORK,
        CLOSING,
        CLOSED
    };

    Session_status status = WORK;
    while (status != CLOSED)
    {
        ibv_cq* cq;
        void* cq_context;
        // Pre-allocated work completions array used for polling
        ibv_wc wc_[CQ_SIZE];

        if (ibv_get_cq_event(event_channel_, &cq, &cq_context))
        {
            log_error("Failed to get cq_event");
            return;
        }

        if (cq != cq_)
        {
            log_error(make_string("CQ event for unknown CQ %p", cq));
            return;
        }

        ibv_ack_cq_events(cq, 1);
    
        if (ibv_req_notify_cq(cq_, 0))
        {
            log_error("Countn't request CQ notification");
            return;
        }

        int ne = ibv_poll_cq(cq_, CQ_SIZE, static_cast<ibv_wc*>(wc_));
        // VAL(ne);
        if (ne > 0)
        for (int i=0;i<ne;i++)
        {
            if (wc_[i].status != IBV_WC_SUCCESS)
            {
                log_error(make_string("Got bad completion with status: 0x%x, vendor syndrome: 0x%x\n", wc_[i].status, wc_[i].vendor_err));
                return;
                // error
            }
            switch(wc_[i].opcode)
            {
                case IBV_WC_RECV_RDMA_WITH_IMM: // Recv Remote RDMA Write Message
                {
                    if (status != WORK) continue;

                    // Which RDMA_Endpoint get this message
                    RDMA_Endpoint* endpoint = reinterpret_cast<RDMA_Endpoint*>(wc_[i].wr_id);
                    // Consumed a ibv_post_recv, so add one
                    endpoint->recv();

                    Message_type msgt = (Message_type)wc_[i].imm_data;
                    log_info(make_string("Message Recv: %s", RDMA_Message::get_message(msgt).data()));
                    switch(msgt)
                    {
                        case RDMA_MESSAGE_READ_OVER:
                        {
                            Message_Content msg = RDMA_Message::parse_message_content((char*)endpoint->channel()->incoming()->buffer());
                            endpoint->channel()->send_message(RDMA_MESSAGE_ACK);

                            RDMA_Buffer* buf = (RDMA_Buffer*)endpoint->find_in_table((uint64_t)msg.buffer_mr.remote_addr);
                            delete buf;

                            break;
                        }
                        case RDMA_MESSAGE_READ_REQUEST:
                        {
                            Message_Content msg = RDMA_Message::parse_message_content((char*)endpoint->channel()->incoming()->buffer());
                            endpoint->channel()->send_message(RDMA_MESSAGE_ACK);

                            RDMA_Buffer* test_new = new RDMA_Buffer(endpoint, pd_, msg.buffer_size);

                            endpoint->insert_to_table((uint64_t)test_new, (uint64_t)msg.buffer_mr.remote_addr
                            );

                            endpoint->read_data(test_new, msg);

                            break;
                        }
                        default:
                            log_error("Unsupported Message Type");
                    }
                    break;
                }
                case IBV_WC_RECV:               // Recv Remote RDMA Send Message
                {
                    // Which RDMA_Endpoint get this message
                    RDMA_Endpoint* endpoint = reinterpret_cast<RDMA_Endpoint*>(wc_[i].wr_id);
                    // Consumed a ibv_post_recv, so add one
                    endpoint->recv();

                    Message_type msgt = (Message_type)wc_[i].imm_data;
                    log_info(make_string("Message Recv: %s", RDMA_Message::get_message(msgt).data()));
                    switch(msgt)
                    {
                        case RDMA_MESSAGE_ACK:
                        {
                            std::thread* work_thread = new std::thread(std::bind(&RDMA_Channel::release_remote, endpoint->channel()));
                            work_thread->detach();

                            break;
                        }
                        case RDMA_MESSAGE_CLOSE:
                        {
                            for (auto endpoint: endpoint_list_)
                                endpoint->channel()->send_message(RDMA_MESSAGE_TERMINATE);
                            status = CLOSING;
                            break;
                        }
                        case RDMA_MESSAGE_TERMINATE:
                            status = CLOSED;
                            endpoint->connected_ = false;
                            break;
                        default:
                            log_error("Unsupported Message Type");
                    }

                    break;
                }
                case IBV_WC_RDMA_WRITE: // Successfully Write RDMA Message or Data
                {
                    
                    log_info(make_string("Message Write Success"));
                    // Which RDMA_Channel send this message/data
                    RDMA_Channel* channel = reinterpret_cast<RDMA_Channel*>(wc_[i].wr_id);

                    // Message sent success, unlock the channel outgoing
                    std::thread* work_thread = new std::thread(std::bind(&RDMA_Channel::release_local, channel));
                    work_thread->detach();

                    break;
                }
                case IBV_WC_SEND:       // Successfully Send RDMA Message
                {
                    // Which RDMA_Channel send this message
                    RDMA_Channel* channel = reinterpret_cast<RDMA_Channel*>(wc_[i].wr_id);
                    log_info(make_string("Message Send Success"));

                    break;
                }
                case IBV_WC_RDMA_READ:  // Successfully Read RDMA Data
                {
                    RDMA_Buffer* rb = reinterpret_cast<RDMA_Buffer*>(wc_[i].wr_id);
                    char* temp = (char*)rb->buffer();
                    //log_ok(make_string("RDMA Read: %s", temp));
                    
                    RDMA_Endpoint* endpoint = rb->endpoint();

                    endpoint->data_recv_success(rb->size());
                    //log_ok(make_string("RDMA Read: %d bytes, total %d bytes", rb->size(), endpoint->total_recv_data()));

                    uint64_t res = endpoint->find_in_table((uint64_t)rb);
                    endpoint->channel()->send_message(RDMA_MESSAGE_READ_OVER, res);

                    delete rb;

                    break;
                }
                default:
                {
                    log_error("Unsupported opcode");
                }
            }
        }
    }

    if (pre_)
    {
        pre_->close_daemon();
    }
}
