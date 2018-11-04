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

enum Channel_status
{
    IDLE,
    LOCK
};

class RDMA_Endpoint;
class RDMA_Buffer;

class RDMA_Channel
{
public:
    RDMA_Channel(RDMA_Endpoint* endpoint, ibv_pd* pd, ibv_qp* qp);
    ~RDMA_Channel();

    void send_message(enum Message_type msgt, uint64_t addr = 0);
    void request_read(RDMA_Buffer* buffer);

    void channel_lock();
    void channel_release_local();
    void channel_release_remote();

    RDMA_Buffer* incoming();
    RDMA_Buffer* outgoing();

    Remote_MR remote_mr_;

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

    std::mutex channel_cv_mutex_;
    std::condition_variable channel_cv_;

    Channel_status local_status_;
    Channel_status remote_status_;
};

#endif // !RDMA_CHANNEL_H