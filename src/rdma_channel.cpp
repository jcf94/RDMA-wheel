/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_CHANNEL_CPP
************************************************ */

#include "rdma_channel.h"
#include "rdma_endpoint.h"
#include "rdma_buffer.h"

#include <thread>

RDMA_Channel::RDMA_Channel(RDMA_Endpoint* endpoint, ibv_pd* pd, ibv_qp* qp)
    : endpoint_(endpoint), qp_(qp), pd_(pd), local_status_(IDLE), remote_status_(IDLE)
{
    // Create Message Buffer ......
    incoming_ = new RDMA_Buffer(endpoint, pd_, kMessageTotalBytes);
    outgoing_ = new RDMA_Buffer(endpoint, pd_, kMessageTotalBytes);

    log_info("RDMA_Channel Created");
}

RDMA_Channel::~RDMA_Channel()
{
    delete incoming_;
    delete outgoing_;

    log_info("RDMA_Channel Deleted");
}

void RDMA_Channel::send_message(Message_type msgt, uint64_t addr)
{
    switch(msgt)
    {
        case RDMA_MESSAGE_ACK:
        case RDMA_MESSAGE_CLOSE:
        case RDMA_MESSAGE_TERMINATE:
        {
            send(msgt, 0);
            break;
        }
        case RDMA_MESSAGE_BUFFER_UNLOCK:
        {
            std::thread* work_thread = new std::thread([this, msgt, addr](){

                channel_lock();

                RDMA_Message::fill_message_content((char*)outgoing_->buffer(), (void*)addr, kRemoteAddrEndIndex, NULL);
                write(msgt, kRemoteAddrEndIndex);
            });
            break;
        }
        default:
            log_error("Unsupported Message Type");
    }
}

void RDMA_Channel::request_read(RDMA_Buffer* buffer)
{
    std::thread* work_thread = new std::thread([this, buffer](){

        channel_lock();

        endpoint_->insert_to_table((uint64_t)buffer->buffer(), (uint64_t)buffer);

        RDMA_Message::fill_message_content((char*)outgoing_->buffer(), buffer->buffer(), buffer->size(), buffer->mr());
        write(RDMA_MESSAGE_READ_REQUEST, kMessageTotalBytes);
    });
}

void RDMA_Channel::write(uint32_t imm_data, size_t size)
{
    struct ibv_sge list;
    list.addr = (uint64_t) outgoing_->buffer(); // Message
    list.length = size; // Message size
    list.lkey = outgoing_->mr()->lkey;

    struct ibv_send_wr wr;
    memset(&wr, 0, sizeof(wr));
    wr.wr_id = (uint64_t) this; // Which RDMA_Channel send this message
    wr.sg_list = &list;
    wr.num_sge = 1;
    wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.imm_data = imm_data;
    wr.wr.rdma.remote_addr = (uint64_t) remote_mr_.remote_addr;
    wr.wr.rdma.rkey = remote_mr_.rkey;

    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(qp_, &wr, &bad_wr))
    {
        log_error("Failed to send message: ibv_post_send error");
    } else
    {
        log_info(make_string("Write Message post: %s", get_message((Message_type)imm_data).data()));
    }
}

void RDMA_Channel::send(uint32_t imm_data, size_t size)
{
    struct ibv_sge list;
    list.addr = (uint64_t) outgoing_->buffer(); // Message
    list.length = size; // Message size
    list.lkey = outgoing_->mr()->lkey;

    struct ibv_send_wr wr;
    memset(&wr, 0, sizeof(wr));
    wr.wr_id = (uint64_t) this; // Which RDMA_Channel send this message
    wr.sg_list = &list;
    wr.num_sge = 1;
    wr.opcode = IBV_WR_SEND_WITH_IMM;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.imm_data = imm_data;

    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(qp_, &wr, &bad_wr))
    {
        log_error("Failed to send message: ibv_post_send error");
    } else
    {
        log_info(make_string("Send Message post: %s", get_message((Message_type)imm_data).data()));
    }
}

void RDMA_Channel::channel_lock()
{
    std::unique_lock<std::mutex> lock(channel_cv_mutex_);
    while (local_status_ == LOCK || remote_status_ == LOCK)
        channel_cv_.wait(lock);
    local_status_ = LOCK;
    remote_status_ = LOCK;
}

void RDMA_Channel::channel_release_local()
{
    std::lock_guard<std::mutex> lock(channel_cv_mutex_);
    local_status_ = IDLE;
    channel_cv_.notify_one();
}

void RDMA_Channel::channel_release_remote()
{
    std::lock_guard<std::mutex> lock(channel_cv_mutex_);
    remote_status_ = IDLE;
    channel_cv_.notify_one();
}

RDMA_Buffer* RDMA_Channel::incoming()
{
    return incoming_;
}

RDMA_Buffer* RDMA_Channel::outgoing()
{
    return outgoing_;
}