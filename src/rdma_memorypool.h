/* ***********************************************
MYID   : Chen Fan
LANG   : G++
PROG   : RDMA_MEMORYPOOL_H
************************************************ */

#ifndef RDMA_MEMORYPOOL_H
#define RDMA_MEMORYPOOL_H

#include "memorypool.h"

class RDMA_MemBlock : public BaseBlock
{
public:
    RDMA_MemBlock(BlockList* blocklist, int size)
        : BaseBlock(blocklist, size)
    {

    }

    ~RDMA_MemBlock()
    {

    }
};

class RDMA_MemoryPool : public MemoryPool
{
public:
    RDMA_MemoryPool();
    ~RDMA_MemoryPool();

private:

};


#endif // !RDMA_MEMORYPOOL_H