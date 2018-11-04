/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_ENDPOINT_H
************************************************ */

#ifndef RDMA_ENDPOINT_H
#define RDMA_ENDPOINT_H

#include <map>

#include "rdma_util.h"

struct RDMA_Endpoint_Info
{
    uint32_t lid;
    uint32_t qpn;
    uint32_t psn;
};

class RDMA_Session;
class RDMA_Channel;
class RDMA_Buffer;

class RDMA_Endpoint
{
public:
    RDMA_Endpoint(RDMA_Session* session, int ib_port);
    ~RDMA_Endpoint();

    struct cm_con_data_t get_local_con_data();
    void connect(struct cm_con_data_t remote_con_data);
    void close();

    void send_data(void* addr, int size);
    void read_data(RDMA_Buffer* buffer, struct Message_Content msg);
    void recv();

    uint64_t find_in_table(uint64_t key, bool erase = true);
    void insert_to_table(uint64_t key, uint64_t value);

    // ----- Private To Public -----
    inline RDMA_Channel* channel() const {return channel_;}

private:
    int modify_qp_to_init();
    int modify_qp_to_rtr();
    int modify_qp_to_rts();

    bool connected_;
    int ib_port_;

    RDMA_Session* session_;
    // Queue pair
    ibv_qp* qp_;
    // Endpoint info used for exchange with remote
    RDMA_Endpoint_Info self_, remote_;
    // Message channel
    RDMA_Channel* channel_;

    std::multimap<uint64_t, uint64_t> map_table_;
};

#endif // !RDMA_ENDPOINT_H