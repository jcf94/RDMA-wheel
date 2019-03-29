/* ***********************************************
MYID   : Chen Fan
LANG   : G++
PROG   : RDMA_MEMORYPOOL_CPP
************************************************ */

#include "rdma_memorypool.h"

class RDMA_MemBlockFactory : public BaseBlockFactory
{
public:
    BaseBlock* create(BlockList* blocklist, int size)
    {
        return new RDMA_MemBlock(blocklist, size);
    }
};

RDMA_MemoryPool::RDMA_MemoryPool()
    : MemoryPool(new RDMA_MemBlockFactory)
{

}

RDMA_MemoryPool::~RDMA_MemoryPool()
{
    
}