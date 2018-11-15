/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_BUFFER_CPP
************************************************ */

#include "rdma_util.h"

#include "rdma_buffer.h"

RDMA_Buffer::RDMA_Buffer(RDMA_Channel* channel, ibv_pd* pd, int size)
    : channel_(channel), size_(size), buffer_owned_(true)
{
    buffer_ = malloc(size);
    mr_ = ibv_reg_mr(pd, buffer_, size_,
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
    if (!mr_)
    {
        log_error("Failed to register memory region");
    }

    log_info("RDMA_Buffer Created");
}

RDMA_Buffer::RDMA_Buffer(RDMA_Channel* channel, ibv_pd* pd, int size, void* addr)
    : buffer_(addr), channel_(channel), size_(size), buffer_owned_(false)
{
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

    if (buffer_owned_)
    {
        free(buffer_);
    }

    log_info("RDMA_Buffer Deleted");
}
