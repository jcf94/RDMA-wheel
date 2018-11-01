/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_SESSION_CPP
************************************************ */

#include "rdma_session.h"
#include <string.h>
#include <stdio.h>
#include <functional>

#define MSG_SIZE 20

RDMA_Session::RDMA_Session(char* dev_name)
{
    dev_name_ = dev_name;

    // init all of the resources, so cleanup will be easy
    if (open_ib_device())
    {
        return;
        //goto cleanup;
    }

    /* allocate Protection Domain */
    pd_ = ibv_alloc_pd(context_);
    if (!pd_)
    {
        log_error("Failed to allocate protection domain");
        //return 1;
    }

    /* Create completion endpoint */
    event_channel_ = ibv_create_comp_channel(context_);
    if (!event_channel_)
    {
        log_error("Failed to create completion endpoint");
        //return 1;
    }

    cq_ = ibv_create_cq(context_, CQ_SIZE, NULL, event_channel_, 0);
    if (!cq_)
    {
        log_error(make_string("failed to create CQ with %u entries", CQ_SIZE));
        //return 1;
    }

    if (ibv_req_notify_cq(cq_, 0))
    {
        log_error("Countn't request CQ notification");
        //goto cleanup;
    }

    // ------- new thread
    //thread_ = new std::thread(std::bind(&RDMA_Adapter::Adapter_processCQ, this));
    thread_.reset(new std::thread(std::bind(&RDMA_Session::session_processCQ, this)));

    log_info("RDMA_Session Created");
}

RDMA_Session::~RDMA_Session()
{
    stop_process();

    for (auto i:endpoint_table)
        delete i;
    
    thread_.reset();
    
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

    log_ok("Starting Resources Initialization");
    log_ok("Searching for IB devices in host");

    // get device names in the system
    dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list)
    {
        log_error("failed to get IB devices list");
        return 1;
    }

    // if there isn't any IB device in host
    if (!num_devices)
    {
        log_error(make_string("found %d device(s)", num_devices));
        return 1;
    }
    log_info(make_string("found %d device(s)", num_devices));

    if (!dev_name_)
    {
        ib_dev = *dev_list;
        if (!ib_dev)
        {
            log_error("No IB devices found");
            return 1;
        }
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
        log_error(make_string("failed to open device %s", dev_name_));
        return 1;
    }

    return 0;
}

RDMA_Endpoint* RDMA_Session::add_connection(RDMA_Pre* pre)
{
    RDMA_Endpoint* new_endpoint = new RDMA_Endpoint(this, pre->config.ib_port);
    endpoint_table.push_back(new_endpoint);

    new_endpoint->connect(pre->exchange_qp_data(new_endpoint->get_local_con_data()));

    return new_endpoint;
}

void RDMA_Session::stop_process()
{
    thread_->join();
}

void RDMA_Session::session_processCQ()
{
    bool doit = true;
    while (doit)
    {
        ibv_cq* cq;
        void* cq_context;

        if (ibv_get_cq_event(event_channel_, &cq, &cq_context))
        {
            log_error("Failed to get cq_event");
        }

        if (cq != cq_)
        {
            log_error(make_string("CQ event for unknown CQ %p", cq));
        }

        ibv_ack_cq_events(cq, 1);
    
        if (ibv_req_notify_cq(cq_, 0))
        {
            log_error("Countn't request CQ notification");
        }

        int ne = ibv_poll_cq(cq_, CQ_SIZE, static_cast<ibv_wc*>(wc_));
        // VAL(ne);
        if (ne > 0)
        for (int i=0;i<ne;i++)
        {
            if (wc_[i].status != IBV_WC_SUCCESS)
            {
                log_error(make_string("got bad completion with status: 0x%x, vendor syndrome: 0x%x\n", wc_[i].status, wc_[i].vendor_err));
                // error
            }
            switch(wc_[i].opcode)
            {
                case IBV_WC_RECV_RDMA_WITH_IMM: // Recv Remote RDMA Message
                {
                    // Which RDMA_Endpoint get this message
                    RDMA_Endpoint* rc = reinterpret_cast<RDMA_Endpoint*>(wc_[i].wr_id);
                    // Consumed a ibv_post_recv, so add one
                    rc->recv();

                    Message_type msgt = (Message_type)wc_[i].imm_data;
                    log_info(make_string("Message Recv: %s", get_message(msgt).data()));
                    switch(msgt)
                    {
                        case RDMA_MESSAGE_ACK:
                            rc->message_->outgoing_->status_ = IDLE;
                            break;
                        case RDMA_MESSAGE_BUFFER_UNLOCK:
                            // 
                            break;
                        case RDMA_MESSAGE_READ_REQUEST:
                        {
                            char* temp = (char*)rc->message_->incoming_->buffer_;
                            Remote_info msg;
                            memcpy(&(msg.buffer_size_), &temp[kBufferSizeStartIndex], 8);
                            memcpy(&(msg.remote_addr_), &temp[kRemoteAddrStartIndex], 8);
                            memcpy(&(msg.rkey_), &temp[kRkeyStartIndex], 4);
                            rc->send_message(RDMA_MESSAGE_ACK);

                            RDMA_Buffer* test_new = new RDMA_Buffer(pd_, msg.buffer_size_);
                            rc->read_data(test_new, msg);

                            // RDMA_READ
                            break;
                        }
                        case RDMA_MESSAGE_CLOSE:
                            rc->send_message(RDMA_MESSAGE_TERMINATE);
                            //doit = false;
                            break;
                        case RDMA_MESSAGE_TERMINATE:
                            doit = false;
                            break;
                        default:
                            ;
                    }
                    break;
                }
                case IBV_WC_RDMA_WRITE: // Send RDMA Message or Data
                {
                    // Which RDMA_Message send this message/data
                    RDMA_Message* rm = reinterpret_cast<RDMA_Message*>(wc_[i].wr_id);
                    log_info(make_string("Message Send Success"));
                    
                    break;
                }
                case IBV_WC_RDMA_READ:
                {
                    RDMA_Buffer* rb = reinterpret_cast<RDMA_Buffer*>(wc_[i].wr_id);
                    char* temp = (char*)rb->buffer_;
                    log_info(make_string("RDMA Read: %s", temp));

                    

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