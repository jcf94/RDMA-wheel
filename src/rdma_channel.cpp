/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_CHANNEL_CPP
************************************************ */

#include "rdma_channel.h"

#include "rdma_util.h"
#include "rdma_message.h"
#include "rdma_endpoint.h"
#include "rdma_buffer.h"
#include "ThreadPool.h"

RDMA_Channel::RDMA_Channel(RDMA_Endpoint* endpoint, ibv_pd* pd, ibv_qp* qp)
    : endpoint_(endpoint), pd_(pd), qp_(qp), local_status_(IDLE), remote_status_(IDLE)
{
    // post recv
    for (int i=0;i<100;i++)
    {
        recv();
    }

    // Create Message Buffer ......
    incoming_ = new RDMA_Buffer(this, pd_, kMessageTotalBytes);
    outgoing_ = new RDMA_Buffer(this, pd_, kMessageTotalBytes);

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

    nolock_pool_ = new ThreadPool(DEFAULT_NOLOCK_POOL_THREADS);
    if (nolock_pool_)
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

    nolock_pool_->wait();
    delete nolock_pool_;

    delete incoming_;
    delete outgoing_;

    log_info("RDMA_Channel Deleted");
}

// ----------------------------------------------

void RDMA_Channel::request_read(RDMA_Buffer* buffer)
{
    task_with_lock([this, buffer]
    {
        insert_to_table((uint64_t)buffer->buffer(), (uint64_t)buffer);

        RDMA_Message::fill_message_content((char*)outgoing_->buffer(), buffer->buffer(), buffer->size(), buffer->mr());
        write(RDMA_MESSAGE_READ_REQUEST, kMessageTotalBytes);
    });
}

// ----------------------------------------------

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

void RDMA_Channel::recv()
{
    struct ibv_recv_wr wr;
    memset(&wr, 0, sizeof(wr));
    wr.wr_id = (uint64_t) this; // Which RDMA_Channel get this message
    struct ibv_recv_wr* bad_wr;
    if (ibv_post_recv(qp_, &wr, &bad_wr))
    {
        log_error("Failed to post recv");
    }
}

void RDMA_Channel::read_data(RDMA_Buffer* buffer, Message_Content msg)
{
    struct ibv_sge list;
    list.addr = (uint64_t) buffer->buffer();
    list.length = msg.buffer_size;
    list.lkey = buffer->mr()->lkey;

    struct ibv_send_wr wr;
    memset(&wr, 0, sizeof(wr));
    wr.wr_id = (uint64_t) buffer; // The RDMA_Buffer used to store the data
    wr.sg_list = &list;
    wr.num_sge = 1;
    wr.opcode = IBV_WR_RDMA_READ;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = (uint64_t) msg.remote_addr;
    wr.wr.rdma.rkey = msg.rkey;

    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(qp_, &wr, &bad_wr))
    {
        log_error("Failed to post send");
    } else
    {
        log_info(make_string("Send Message post: RDMA_READ"));
    }
}

// ----------------------------------------------

bool RDMA_Channel::data_recv_success(int size)
{
    total_recv_data_ += size;
    return total_recv_data_ == target_recv_data_;
}

void RDMA_Channel::target_count_set(uint64_t size)
{
    log_info(make_string("Recv Count: %lld", total_recv_data_));
    total_recv_data_ = 0;
    target_recv_data_ = size;
}

// ----------------------------------------------

uint64_t RDMA_Channel::find_in_table(uint64_t key, bool erase)
{
    std::lock_guard<std::mutex> lock(map_lock_);
    auto temp = map_table_.find(key);
    if (temp == map_table_.end())
    {
        log_error("Request key not found in map_table");
        return 0;
    } else
    {
        //log_warning(make_string("Find %p %p", temp->first, temp->second));
        uint64_t res = temp->second;
        // Erase is important, if two RDMA_Buffer has same address
        // the table find result will be a out of date value
        if (erase) map_table_.erase(temp);
        return res;
    }
}

void RDMA_Channel::insert_to_table(uint64_t key, uint64_t value)
{
    std::lock_guard<std::mutex> lock(map_lock_);
    //log_warning(make_string("Insert %p %p", key, value));
    auto temp = map_table_.find(key);
    if (temp != map_table_.end()) {log_error("already have");}
    map_table_.insert(std::make_pair(key, value));
}

int RDMA_Channel::get_table_size()
{
    std::lock_guard<std::mutex> lock(map_lock_);
    //log_ok(map_table_.size());
    return map_table_.size();
}

// ----------------------------------------------

void RDMA_Channel::task_with_lock(std::function<void()> &&task)
{
    work_pool_->add_task([this, task_run = std::move(task)]
    {
        std::unique_lock<std::mutex> lock(channel_cv_mutex_);
        channel_cv_.wait(lock, [this]{return local_status_ != LOCK && remote_status_ != LOCK;});
        local_status_ = LOCK;
        remote_status_ = LOCK;

        task_run();
    });
}

void RDMA_Channel::task_without_lock(std::function<void()> &&task)
{
    nolock_pool_->add_task([this, task_run = std::move(task)]
    {
        task_run();
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

bool RDMA_Channel::work_pool_clear()
{
    return work_pool_->tasks_in_queue() == 0;
}