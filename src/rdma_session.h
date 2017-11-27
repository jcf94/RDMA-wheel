/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_SESSION_H
************************************************ */

#ifndef RDMA_SESSION_H
#define RDMA_SESSION_H

#include "rdma_util.h"
#include "rdma_endpoint.h"
#include "rdma_pre.h"

class RDMA_Endpoint;
class RDMA_Pre;

class RDMA_Session
{
public:

    friend class RDMA_Endpoint;

    RDMA_Session(RDMA_Pre* pre);
    ~RDMA_Session();

    void stop_process();
    
    const static int CQ_SIZE = 1000;
    // 
    RDMA_Endpoint* endpoint_;

private:
    int open_ib_device();
    void session_processCQ();

    // device handle
    ibv_context* context_;
    // ibverbs protection domain
    ibv_pd* pd_;
    // Completion event endpoint, to wait for work completions
    ibv_comp_channel* event_channel_;
    // Completion queue, to poll on work completions
    ibv_cq* cq_;
    // Pre-allocated work completions array used for polling
    ibv_wc wc_[CQ_SIZE];

    //std::thread* thread_;
    std::unique_ptr<std::thread> thread_;

    config_t config;
};

#endif // !RDMA_SESSION_H