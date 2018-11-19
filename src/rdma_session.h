/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_SESSION_H
************************************************ */

// class RDMA_Session
// 
// Responsible for IB device management, holds multi Endpoint

#ifndef RDMA_SESSION_H
#define RDMA_SESSION_H

#include <thread>
#include <vector>
#include <infiniband/verbs.h>

class RDMA_Endpoint;
class RDMA_Pre;

enum Session_status
{
    WORK,
    CLOSING,
    CLOSED
};

class RDMA_Session
{
public:
    RDMA_Session(char* dev_name = NULL);
    ~RDMA_Session();

    void stop_process();

    RDMA_Endpoint* new_endpoint(RDMA_Pre* pre);
    void delete_endpoint(RDMA_Endpoint* endpoint);

    RDMA_Endpoint* ptp_connect(RDMA_Pre* pre);
    void daemon_connect(RDMA_Pre* pre);

    const static int CQ_SIZE = 1000;

    // ----- Private To Public -----
    inline RDMA_Pre* pre() const {return pre_;}

    // Session status
    Session_status status_;

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
    // Pre connection used to establish RDMA link
    RDMA_Pre* pre_ = NULL;
    // List of endpoints
    std::vector<RDMA_Endpoint*> endpoint_list_;
    // Thread used to process CQ
    std::unique_ptr<std::thread> process_thread_;
};

#endif // !RDMA_SESSION_H