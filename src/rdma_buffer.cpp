/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_BUFFER_CPP
************************************************ */

#include "rdma_buffer.h"

RDMA_Buffer::RDMA_Buffer(ibv_pd* pd, int size)
    : size_(size), status_(IDLE)
{
    buffer_ = malloc(size_);
    mr_ = ibv_reg_mr(pd, buffer_, size_,
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
    if (!mr_)
    {
        log_error("Failed to register memory region");
    }

    log_info("RDMA_Buffer Created");
}

RDMA_Buffer::~RDMA_Buffer()
{
    if (ibv_dereg_mr(mr_))
    {
        log_error("ibv_dereg_mr failed");
    }

    log_info("RDMA_Buffer Deleted");
}