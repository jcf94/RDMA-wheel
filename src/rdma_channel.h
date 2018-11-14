/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_CHANNEL_H
************************************************ */

#ifndef RDMA_CHANNEL_H
#define RDMA_CHANNEL_H

#include <mutex>
#include <condition_variable>

#include "rdma_message.h"

#define DEFAULT_POOL_THREADS 8

enum Channel_status
{
    IDLE,
    LOCK
};

class RDMA_Endpoint;
class RDMA_Buffer;
class ThreadPool;

class RDMA_Channel
{
public:
    RDMA_Channel(RDMA_Endpoint* endpoint, ibv_pd* pd, ibv_qp* qp);
    ~RDMA_Channel();

    void send_message(enum Message_type msgt, uint64_t addr = 0);
    void request_read(RDMA_Buffer* buffer);

    void lock();
    void release_local();
    void release_remote();

    // ----- Private To Public -----
    inline RDMA_Buffer* incoming() const {return incoming_;}
    inline RDMA_Buffer* outgoing() const {return outgoing_;}

    Buffer_MR remote_mr_;

private:
    void write(uint32_t imm_data, size_t size);
    void send(uint32_t imm_data, size_t size);

    RDMA_Endpoint* endpoint_;
    ibv_pd* pd_;
    ibv_qp* qp_;

    // Message incoming buffer, only read
    RDMA_Buffer* incoming_;
    // Message outgoing buffer
    RDMA_Buffer* outgoing_;
    // Whether it's ready to recv incoming message
    Channel_status remote_status_;
    // Whether it's ready to send outgoing message
    Channel_status local_status_;
    // cv & mutex to lock the remote/local status
    std::mutex channel_cv_mutex_;
    std::condition_variable channel_cv_;
    // ThreadPool used for data process
    ThreadPool* pool_ = NULL;
};

#endif // !RDMA_CHANNEL_H