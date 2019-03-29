/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_ENDPOINT_H
************************************************ */

// class RDMA_Endpoint
// 
// Responsible for connection establishment, hide RDMA low level details in Channel

#ifndef RDMA_ENDPOINT_H
#define RDMA_ENDPOINT_H

#include <infiniband/verbs.h>
#include <set>

struct RDMA_Endpoint_Info
{
    uint32_t lid;
    uint32_t qpn;
    uint32_t psn;
};

class RDMA_Channel;
class RDMA_Buffer;
class RDMA_Session;

class RDMA_Endpoint
{
public:
    RDMA_Endpoint(ibv_pd* pd, ibv_cq* cq, ibv_context* context, int ib_port, int cq_size, RDMA_Session* session);
    ~RDMA_Endpoint();

    struct cm_con_data_t get_local_con_data();
    void connect(struct cm_con_data_t remote_con_data);
    void close();

    void send_data(void* addr, int size);

    void set_sync_barrier(int size);
    void wait_for_sync();

    // ----- Private To Public -----
    inline RDMA_Channel* channel() const {return channel_;}
    inline RDMA_Session* session() const {return session_;}

    bool connected_;

private:
    int modify_qp_to_init();
    int modify_qp_to_rtr();
    int modify_qp_to_rts();

    int ib_port_;

    // Protection domain
    ibv_pd* pd_;
    // Queue pair
    ibv_qp* qp_;

    // Endpoint info used for exchange with remote
    RDMA_Endpoint_Info self_, remote_;
    // Message channel
    RDMA_Channel* channel_ = NULL;
    // Session
    RDMA_Session* session_ = NULL;
};

#endif // !RDMA_ENDPOINT_H