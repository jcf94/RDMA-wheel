/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_BUFFER_H
************************************************ */

// class RDMA_Buffer
// 
// Responsible for RDMA registed buffer manager

#ifndef RDMA_BUFFER_H
#define RDMA_BUFFER_H

#include "rdma_memorypool.h"

#include <infiniband/verbs.h>

class RDMA_Channel;

class RDMA_Buffer
{
public:
    RDMA_Buffer(RDMA_Channel* channel, ibv_pd* pd, int size, void* addr = NULL);
    ~RDMA_Buffer();

    // ----- Private To Public -----
    inline RDMA_Channel* channel() const {return channel_;}
    inline void* buffer() const {return buffer_;}
    inline uint64_t size() const {return size_;}
    inline ibv_mr* mr() const {return mr_;}

private:
    bool buffer_owned_;
    RDMA_Channel* channel_ = NULL;

    void* buffer_ = NULL;
    uint64_t size_;
    ibv_mr* mr_;
    RDMA_MemBlock* memblock_;
};

#endif // !RDMA_BUFFER_H