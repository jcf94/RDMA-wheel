/* ***********************************************
MYID   : Chen Fan
LANG   : G++
PROG   : RDMA_MEMORYPOOL_CPP
************************************************ */

#include "rdma_memorypool.h"
#include "rdma_util.h"

RDMA_MemBlock::RDMA_MemBlock(BlockList* blocklist, int size,
    RDMA_MemoryPool* mempool, ibv_pd* pd)
        : BaseBlock(blocklist, size), mempool_(mempool)
    {
        mr_ = ibv_reg_mr(pd, this->dataaddr(), this->size(),
            IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
        if (!mr_)
        {
            log_error("Failed to register memory region");
        }

        log_info("RDMA_MemBlock Created");
    }

RDMA_MemBlock::~RDMA_MemBlock()
{
    if (ibv_dereg_mr(mr_))
    {
        log_error("ibv_dereg_mr failed");
    }
    log_info("RDMA_MemBlock Deleted");
}

class RDMA_MemBlockFactory : public BaseBlockFactory
{
public:
    RDMA_MemBlockFactory(RDMA_MemoryPool* mempool, ibv_pd* pd)
        : mempool_(mempool), pd_(pd) {}

    BaseBlock* create(BlockList* blocklist, int size)
    {
        return new RDMA_MemBlock(blocklist, size, mempool_, pd_);
    }

private:
    ibv_pd* pd_;
    RDMA_MemoryPool* mempool_;
};

RDMA_MemoryPool::RDMA_MemoryPool(ibv_pd* pd)
    : MemoryPool(new RDMA_MemBlockFactory(this, pd)),
    pd_(pd)
{
    log_info("RDMA_MemoryPool Created");
}

RDMA_MemoryPool::~RDMA_MemoryPool()
{
    log_info("RDMA_MemoryPool Deleted");
}