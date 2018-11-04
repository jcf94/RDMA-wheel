/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_SESSION_H
************************************************ */

#ifndef RDMA_SESSION_H
#define RDMA_SESSION_H

#include "rdma_util.h"

class RDMA_Endpoint;
class RDMA_Pre;

class RDMA_Session
{
public:
    RDMA_Session(char* dev_name = NULL);
    ~RDMA_Session();

    RDMA_Endpoint* add_connection(RDMA_Pre* pre);
    void stop_process();

    ibv_pd* pd();
    ibv_cq* cq();
    ibv_context* context();

    const static int CQ_SIZE = 1000;

private:
    int open_ib_device();
    void session_processCQ();

    char* dev_name_;
    // device handle
    ibv_context* context_;
    // ibverbs protection domain
    ibv_pd* pd_;
    // Completion event endpoint, to wait for work completions
    ibv_comp_channel* event_channel_;
    // Completion queue, to poll on work completions
    ibv_cq* cq_;
    // List of endpoints
    std::vector<RDMA_Endpoint*> endpoint_list_;
    // Thread used to process CQ
    std::unique_ptr<std::thread> process_thread_;
};

#endif // !RDMA_SESSION_H