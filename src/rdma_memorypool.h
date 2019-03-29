/* ***********************************************
MYID   : Chen Fan
LANG   : G++
PROG   : RDMA_MEMORYPOOL_H
************************************************ */

#ifndef RDMA_MEMORYPOOL_H
#define RDMA_MEMORYPOOL_H

#include "memorypool.h"

#include <infiniband/verbs.h>

class RDMA_MemoryPool;

class RDMA_MemBlock : public BaseBlock
{
public:
    RDMA_MemBlock(BlockList* blocklist, int size, RDMA_MemoryPool* mempool,
        ibv_pd* pd);
    ~RDMA_MemBlock();

    inline ibv_mr* mr() const {return mr_;}

private:
    RDMA_MemoryPool* mempool_;
    ibv_mr* mr_;
};

class RDMA_MemoryPool : public MemoryPool
{
public:
    RDMA_MemoryPool(ibv_pd* pd);
    ~RDMA_MemoryPool();

private:
    ibv_pd* pd_;
};


#endif // !RDMA_MEMORYPOOL_H