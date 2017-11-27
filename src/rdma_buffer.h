/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_BUFFER_H
************************************************ */

#ifndef RDMA_BUFFER_H
#define RDMA_BUFFER_H

#include "rdma_util.h"

enum Buffer_status
{
    NONE,
    IDLE,
    LOCK
};

class RDMA_Buffer
{
public:
    friend class RDMA_Pre;
    friend class RDMA_Endpoint;
    friend class RDMA_Message;
    friend class RDMA_Session;

    RDMA_Buffer(ibv_pd* pd, int size);
    ~RDMA_Buffer();

private:

    void* buffer_ = NULL;
    uint64_t size_;
    ibv_mr* mr_;
    Buffer_status status_;
};

#endif // !RDMA_BUFFER_H