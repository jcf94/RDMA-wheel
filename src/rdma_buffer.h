/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_BUFFER_H
************************************************ */

#ifndef RDMA_BUFFER_H
#define RDMA_BUFFER_H

#include "rdma_util.h"

class RDMA_Endpoint;

class RDMA_Buffer
{
public:
    friend class TCP_Sock_Pre;
    friend class RDMA_Endpoint;
    friend class RDMA_Channel;
    friend class RDMA_Session;

    RDMA_Buffer(RDMA_Endpoint* endpoint, ibv_pd* pd, int size);
    ~RDMA_Buffer();

private:

    RDMA_Endpoint* endpoint_;

    void* buffer_ = NULL;
    uint64_t size_;
    ibv_mr* mr_;
};

#endif // !RDMA_BUFFER_H