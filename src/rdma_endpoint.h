/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_ENDPOINT_H
************************************************ */

#ifndef RDMA_ENDPOINT_H
#define RDMA_ENDPOINT_H

#include <map>

#include "rdma_session.h"
#include "rdma_message.h"

class RDMA_Buffer;
class RDMA_Session;

class RDMA_Endpoint
{
public:
    friend class RDMA_Pre;
    friend class RDMA_Buffer;
    friend class RDMA_Message;
    friend class RDMA_Session;

    RDMA_Endpoint(RDMA_Session* session, int ib_port);
    ~RDMA_Endpoint();

    void connect();
    void recv();
    void send_message(Message_type msgt);
    void read_data(RDMA_Buffer* buffer, Remote_info msg);

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
    // Message Buffer
    RDMA_Message* message_;
    //std::map<RDMA_Buffer*> buffer_table_;
};

#endif // !RDMA_ENDPOINT_H