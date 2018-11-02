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
#include "rdma_channel.h"
#include "rdma_endpoint.h"
#include "rdma_pre.h"

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

    log_info("RDMA_Session Created");
}

RDMA_Session::~RDMA_Session()
{
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

RDMA_Endpoint* RDMA_Session::add_connection(RDMA_Pre* pre)
{
    RDMA_Endpoint* new_endpoint = new RDMA_Endpoint(this, pre->config.ib_port);
    endpoint_list_.push_back(new_endpoint);

    new_endpoint->connect(pre->exchange_qp_data(new_endpoint->get_local_con_data()));

    return new_endpoint;
}

void RDMA_Session::stop_process()
{
    process_thread_->join();
}

enum Session_status
{
    WORK,
    CLOSING,
    CLOSED
};

void RDMA_Session::session_processCQ()
{
    Session_status status = WORK;
    while (status == WORK || status == CLOSING)
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
                    log_info(make_string("Message Recv: %s", get_message(msgt).data()));
                    switch(msgt)
                    {
                        case RDMA_MESSAGE_BUFFER_UNLOCK:
                        {
                            char* temp = (char*)endpoint->channel_->incoming_->buffer_;
                            Remote_info msg;
                            memcpy(&(msg.remote_addr_), &temp[kRemoteAddrStartIndex], 8);
                            endpoint->channel_->send_message(RDMA_MESSAGE_ACK);

                            RDMA_Buffer* buf = (RDMA_Buffer*)endpoint->find_in_table((uint64_t)msg.remote_addr_);
                            delete buf;

                            break;
                        }
                        case RDMA_MESSAGE_READ_REQUEST:
                        {
                            char* temp = (char*)endpoint->channel_->incoming_->buffer_;
                            Remote_info msg;
                            memcpy(&(msg.buffer_size_), &temp[kBufferSizeStartIndex], 8);
                            memcpy(&(msg.remote_addr_), &temp[kRemoteAddrStartIndex], 8);
                            memcpy(&(msg.rkey_), &temp[kRkeyStartIndex], 4);
                            endpoint->channel_->send_message(RDMA_MESSAGE_ACK);

                            RDMA_Buffer* test_new = new RDMA_Buffer(endpoint, pd_, msg.buffer_size_);

                            endpoint->insert_to_table((uint64_t)test_new, (uint64_t)msg.remote_addr_
                            );

                            endpoint->read_data(test_new, msg);

                            // RDMA_READ
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
                    log_info(make_string("Message Recv: %s", get_message(msgt).data()));
                    switch(msgt)
                    {
                        case RDMA_MESSAGE_ACK:
                        {
                            RDMA_Channel* channel = endpoint->channel_;

                            std::thread* work_thread = new std::thread([channel](){
                                std::lock_guard<std::mutex> lock(channel->channel_cv_mutex_);
                                channel->remote_status_ = IDLE;
                                channel->channel_cv_.notify_one();
                            });

                            break;
                        }
                        case RDMA_MESSAGE_CLOSE:
                            endpoint->channel_->send_message(RDMA_MESSAGE_TERMINATE);
                            status = CLOSING;
                            break;
                        case RDMA_MESSAGE_TERMINATE:
                            status = CLOSED;
                            break;
                        default:
                            log_error("Unsupported Message Type");
                    }

                    break;
                }
                case IBV_WC_RDMA_WRITE: // Successfully Write RDMA Message or Data
                {
                    // Which RDMA_Channel send this message/data
                    RDMA_Channel* channel = reinterpret_cast<RDMA_Channel*>(wc_[i].wr_id);
                    // Message sent success, unlock the channel outgoing
                    log_info(make_string("Message Write Success"));

                    std::thread* work_thread = new std::thread([channel](){
                        std::lock_guard<std::mutex> lock(channel->channel_cv_mutex_);
                        channel->local_status_ = IDLE;
                        channel->channel_cv_.notify_one();
                    });

                    break;
                }
                case IBV_WC_SEND:       // Successfully Send RDMA Message
                {
                    // Which RDMA_Channel send this message/data
                    RDMA_Channel* channel = reinterpret_cast<RDMA_Channel*>(wc_[i].wr_id);
                    log_info(make_string("Message Send Success"));

                    break;
                }
                case IBV_WC_RDMA_READ:  // Successfully Read RDMA Data
                {
                    RDMA_Buffer* rb = reinterpret_cast<RDMA_Buffer*>(wc_[i].wr_id);
                    char* temp = (char*)rb->buffer_;
                    log_ok(make_string("RDMA Read: %s", temp));

                    uint64_t res = rb->endpoint_->find_in_table((uint64_t)rb);
                    //log_error(res);
                    rb->endpoint_->channel_->send_message(RDMA_MESSAGE_BUFFER_UNLOCK, res);

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
}

ibv_pd* RDMA_Session::pd()
{
    return pd_;
}