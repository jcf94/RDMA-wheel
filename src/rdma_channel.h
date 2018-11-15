/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_CHANNEL_H
************************************************ */

#ifndef RDMA_CHANNEL_H
#define RDMA_CHANNEL_H

#include <mutex>
#include <condition_variable>
#include <functional>
#include <infiniband/verbs.h>

#define DEFAULT_TOTAL_POOL_THREADS 8
#define DEFAULT_WORK_POOL_THREADS 6
#define DEFAULT_UNLOCK_POOL_THREADS (DEFAULT_TOTAL_POOL_THREADS - DEFAULT_WORK_POOL_THREADS)

enum Channel_status
{
    IDLE,
    LOCK
};

class RDMA_Endpoint;
class RDMA_Buffer;
class ThreadPool;

struct Buffer_MR;

class RDMA_Channel
{
public:
    RDMA_Channel(RDMA_Endpoint* endpoint, ibv_pd* pd, ibv_qp* qp);
    ~RDMA_Channel();

    void request_read(RDMA_Buffer* buffer);

    void write(uint32_t imm_data, size_t size);
    void send(uint32_t imm_data, size_t size);

    void lock(std::function<void()> task);
    void release_local();
    void release_remote();

    // ----- Private To Public -----
    inline ibv_pd* pd() const {return pd_;}
    inline RDMA_Buffer* incoming() const {return incoming_;}
    inline RDMA_Buffer* outgoing() const {return outgoing_;}
    inline ThreadPool* work_pool() const {return work_pool_;}

    Buffer_MR remote_mr_;

private:

    RDMA_Endpoint* endpoint_;
    ibv_pd* pd_;
    ibv_qp* qp_;

    // Message incoming buffer, only read
    RDMA_Buffer* incoming_ = NULL;
    // Message outgoing buffer
    RDMA_Buffer* outgoing_ = NULL;
    // Whether it's ready to recv incoming message
    Channel_status remote_status_;
    // Whether it's ready to send outgoing message
    Channel_status local_status_;
    // cv & mutex to lock the remote/local status
    std::mutex channel_cv_mutex_;
    std::condition_variable channel_cv_;
    // ThreadPool used for data process
    ThreadPool* work_pool_ = NULL;
    ThreadPool* unlock_pool_ = NULL;
};

#endif // !RDMA_CHANNEL_H