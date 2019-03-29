/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_BUFFER_CPP
************************************************ */

#include "rdma_buffer.h"

#include "rdma_session.h"
#include "rdma_channel.h"
#include "rdma_endpoint.h"
#include "rdma_memorypool.h"
#include "rdma_util.h"

RDMA_Buffer::RDMA_Buffer(RDMA_Channel* channel, ibv_pd* pd, int size, void* addr)
    : channel_(channel), size_(size)
{
    if (addr != NULL)
    {
        buffer_ = addr;
        buffer_owned_ = false;
        mr_ = ibv_reg_mr(pd, buffer_, size_,
            IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
        if (!mr_)
        {
            log_error("Failed to register memory region");
        }
    } else
    {
        log_info("ddddddddddddddddddddd");
        //buffer_ = malloc(size);
        memblock_ = (RDMA_MemBlock*)channel_->endpoint()->session()->mempool()->blockalloc(size);
        buffer_ = memblock_->dataaddr();
        mr_ = memblock_->mr();
        buffer_owned_ = true;
        channel_->endpoint()->session()->mempool()->travel();
    }

    log_info("RDMA_Buffer Created");
}

RDMA_Buffer::~RDMA_Buffer()
{
    if (buffer_owned_)
    {
        //free(buffer_);
        memblock_->free();
        channel_->endpoint()->session()->mempool()->travel();
    } else
    {
        if (ibv_dereg_mr(mr_))
        {
            log_error("ibv_dereg_mr failed");
        }
    }

    log_info("RDMA_Buffer Deleted");
}
