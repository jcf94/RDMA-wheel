/* ***********************************************
MYID   : Chen Fan
LANG   : G++
PROG   : RDMA_PRE_H
************************************************ */

#ifndef RDMA_PRE_H
#define RDMA_PRE_H

/* structure of test parameters */
struct config_t
{
    char *server_name;  /* daemon host name */
    int tcp_port;       /* daemon TCP port */
    int ib_port;        /* local IB port to work with */
};

class RDMA_Session;
class RDMA_Endpoint;

class RDMA_Pre
{
public:
    RDMA_Pre() {}
    virtual ~RDMA_Pre() {}

    virtual RDMA_Endpoint* ptp_connect(RDMA_Session* session) = 0;
    virtual void daemon_connect(RDMA_Session* session) = 0;

    config_t config = {
        ((char*)0),     // server_name
        23333,          // tcp_port
        1               // ib_port
    };
};

#endif // !RDMA_PRE_H