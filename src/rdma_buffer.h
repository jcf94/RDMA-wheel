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
    RDMA_Buffer(RDMA_Endpoint* endpoint, ibv_pd* pd, int size);
    RDMA_Buffer(RDMA_Endpoint* endpoint, ibv_pd* pd, int size, void* addr);
    ~RDMA_Buffer();

    // ----- Private To Public -----
    inline RDMA_Endpoint* endpoint() const {return endpoint_;}
    inline void* buffer() const {return buffer_;}
    inline uint64_t size() const {return size_;}
    inline ibv_mr* mr() const {return mr_;}

private:
    bool buffer_owned_;
    RDMA_Endpoint* endpoint_;

    void* buffer_ = NULL;
    uint64_t size_;
    ibv_mr* mr_;
};

#endif // !RDMA_BUFFER_H