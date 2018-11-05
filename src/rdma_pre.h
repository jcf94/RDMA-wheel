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

class RDMA_Pre
{
public:
    RDMA_Pre() {}
    virtual ~RDMA_Pre() {}

    virtual void ptp_connect() = 0;
    // When new connection is estiblished, run connect_callback() to do RDMA init
    virtual void daemon_connect(std::function<void()> connect_callback) = 0;
    virtual void close_daemon() = 0;
    // Exchange QP info to establish the connection
    virtual struct cm_con_data_t exchange_qp_data(struct cm_con_data_t local_con_data) = 0;

    config_t config = {
        ((char*)0),     // server_name
        23333,          // tcp_port
        1               // ib_port
    };
};

#endif // !RDMA_PRE_H