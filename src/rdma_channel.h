/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_CHANNEL_H
************************************************ */

// class RDMA_Channel
// 
// Responsible for RDMA low level data transform

#ifndef RDMA_CHANNEL_H
#define RDMA_CHANNEL_H

#include <map>
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

struct Buffer_MR
{
    uint64_t remote_addr;
    uint32_t rkey;
};

class RDMA_Channel
{
public:
    RDMA_Channel(RDMA_Endpoint* endpoint, ibv_pd* pd, ibv_qp* qp);
    ~RDMA_Channel();

    void request_read(RDMA_Buffer* buffer);

    void write(uint32_t imm_data, size_t size);
    void send(uint32_t imm_data, size_t size);
    void recv();
    void read_data(RDMA_Buffer* buffer, struct Message_Content msg);

    bool data_recv_success(int size);
    void target_count_set(uint64_t size);

    uint64_t find_in_table(uint64_t key, bool erase = true);
    void insert_to_table(uint64_t key, uint64_t value);

    void task_with_lock(std::function<void()> task);
    void release_local();
    void release_remote();
    bool work_pool_clear();

    // ----- Private To Public -----
    inline ibv_pd* pd() const {return pd_;}
    inline RDMA_Endpoint* endpoint() const {return endpoint_;}
    inline RDMA_Buffer* incoming() const {return incoming_;}
    inline RDMA_Buffer* outgoing() const {return outgoing_;}

    Buffer_MR remote_mr_;

private:

    RDMA_Endpoint* endpoint_ = NULL;

    // Protection domain
    ibv_pd* pd_;
    // Queue pair
    ibv_qp* qp_;

    std::multimap<uint64_t, uint64_t> map_table_;
    std::mutex map_lock_;

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

    //Data count
    uint64_t total_recv_data_ = 0;
    uint64_t target_recv_data_ = 0;
};

#endif // !RDMA_CHANNEL_H