/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_CHANNEL_CPP
************************************************ */

#include "rdma_util.h"

#include "rdma_message.h"
#include "rdma_channel.h"
#include "rdma_endpoint.h"
#include "rdma_buffer.h"
#include "../utils/ThreadPool/src/ThreadPool.h"

RDMA_Channel::RDMA_Channel(RDMA_Endpoint* endpoint, ibv_pd* pd, ibv_qp* qp)
    : endpoint_(endpoint), qp_(qp), local_status_(IDLE), remote_status_(IDLE)
{
    // Create Message Buffer ......
    incoming_ = new RDMA_Buffer(endpoint, pd, kMessageTotalBytes);
    outgoing_ = new RDMA_Buffer(endpoint, pd, kMessageTotalBytes);

    work_pool_ = new ThreadPool(DEFAULT_WORK_POOL_THREADS);
    if (work_pool_)
    {
        log_info("ThreadPool Created");
    } else
    {
        log_error("Failed to create ThreadPool");
        return;
    }

    unlock_pool_ = new ThreadPool(DEFAULT_UNLOCK_POOL_THREADS);
    if (unlock_pool_)
    {
        log_info("ThreadPool Created");
    } else
    {
        log_error("Failed to create ThreadPool");
        return;
    }

    log_info("RDMA_Channel Created");
}

RDMA_Channel::~RDMA_Channel()
{
    work_pool_->wait();
    delete work_pool_;

    unlock_pool_->wait();
    delete unlock_pool_;

    delete incoming_;
    delete outgoing_;

    log_info("RDMA_Channel Deleted");
}

void RDMA_Channel::request_read(RDMA_Buffer* buffer)
{
    task_with_lock([this, buffer]
    {
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
        log_info(make_string("Write Message post: %s", RDMA_Message::get_message((Message_type)imm_data).data()));
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
        log_info(make_string("Send Message post: %s", RDMA_Message::get_message((Message_type)imm_data).data()));
    }
}

void RDMA_Channel::task_with_lock(std::function<void()> task)
{
    work_pool_->add_task([this, task]
    {
        std::unique_lock<std::mutex> lock(channel_cv_mutex_);
        channel_cv_.wait(lock, [this]{return local_status_ != LOCK && remote_status_ != LOCK;});
        local_status_ = LOCK;
        remote_status_ = LOCK;

        task();
    });
}

void RDMA_Channel::release_local()
{
    unlock_pool_->add_task([this]
    {
        std::lock_guard<std::mutex> lock(channel_cv_mutex_);
        local_status_ = IDLE;
        channel_cv_.notify_one();
    });
}

void RDMA_Channel::release_remote()
{
    unlock_pool_->add_task([this]
    {
        std::lock_guard<std::mutex> lock(channel_cv_mutex_);
        remote_status_ = IDLE;
        channel_cv_.notify_one();
    });
}
