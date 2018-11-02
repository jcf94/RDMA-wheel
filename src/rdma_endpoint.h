/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_ENDPOINT_H
************************************************ */

#ifndef RDMA_ENDPOINT_H
#define RDMA_ENDPOINT_H

#include <map>

#include "rdma_util.h"
#include "rdma_message.h"

class RDMA_Session;
class RDMA_Channel;
class RDMA_Buffer;

class RDMA_Endpoint
{
public:
    friend class RDMA_Buffer;
    friend class RDMA_Session;

    RDMA_Endpoint(RDMA_Session* session, int ib_port);
    ~RDMA_Endpoint();

    struct cm_con_data_t get_local_con_data();
    void connect(struct cm_con_data_t remote_con_data);
    void recv();

    void send_message(enum Message_type msgt);
    void read_data(RDMA_Buffer* buffer, struct Remote_info msg);

    uint64_t find_in_table(uint64_t key);
    void insert_to_table(uint64_t key, uint64_t value);

private:
    int modify_qp_to_init();
    int modify_qp_to_rtr();
    int modify_qp_to_rts();

    bool connected_;
    int ib_port_;

    RDMA_Session* session_;
    // Queue Pair
    ibv_qp* qp_;

    RDMA_Address self_, remote_;
    // Message channel
    RDMA_Channel* channel_;

    std::map<uint64_t, uint64_t> map_table_;
};

#endif // !RDMA_ENDPOINT_H