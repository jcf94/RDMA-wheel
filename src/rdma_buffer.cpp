/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_BUFFER_CPP
************************************************ */

#include "rdma_buffer.h"

RDMA_Buffer::RDMA_Buffer(RDMA_Endpoint* endpoint, ibv_pd* pd, int size)
    : endpoint_(endpoint), size_(size), buffer_owned_(true)
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

RDMA_Buffer::RDMA_Buffer(RDMA_Endpoint* endpoint, ibv_pd* pd, int size, void* addr)
    : buffer_(addr), endpoint_(endpoint), size_(size), buffer_owned_(false)
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

    if (buffer_owned_) free(buffer_);

    log_info("RDMA_Buffer Deleted");
}

RDMA_Endpoint* RDMA_Buffer::endpoint()
{
    return endpoint_;
}

uint64_t RDMA_Buffer::size()
{
    return size_;
}

void* RDMA_Buffer::buffer()
{
    return buffer_;
}

ibv_mr* RDMA_Buffer::mr()
{
    return mr_;
}