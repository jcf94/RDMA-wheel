/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_ENDPOINT_H
************************************************ */

#ifndef RDMA_ENDPOINT_H
#define RDMA_ENDPOINT_H

#include <infiniband/verbs.h>

struct RDMA_Endpoint_Info
{
    uint32_t lid;
    uint32_t qpn;
    uint32_t psn;
};

class RDMA_Channel;

class RDMA_Endpoint     // Responsible for connection establishment, hide RDMA low level details in Channel
{
public:
    RDMA_Endpoint(ibv_pd* pd, ibv_cq* cq, ibv_context* context, int ib_port, int cq_size);
    ~RDMA_Endpoint();

    struct cm_con_data_t get_local_con_data();
    void connect(struct cm_con_data_t remote_con_data);
    void close();

    void send_data(void* addr, int size);

    void data_send_success(int size);
    bool data_recv_success(int size);
    void target_count_set(uint64_t size);

    void prepare_to_sync(int size);
    void wait_for_sync();

    // ----- Private To Public -----
    inline RDMA_Channel* channel() const {return channel_;}
    inline uint64_t total_send_data() const {return total_send_data_;}
    inline uint64_t total_recv_data() const {return total_recv_data_;}

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

    //Data count
    uint64_t total_send_data_ = 0;
    uint64_t total_recv_data_ = 0;
    uint64_t target_recv_data_ = 0;
};

#endif // !RDMA_ENDPOINT_H