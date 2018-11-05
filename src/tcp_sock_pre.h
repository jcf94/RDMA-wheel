/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: TCP_SOCK_PRE_H
************************************************ */

#ifndef TCP_SOCK_PRE_H
#define TCP_SOCK_PRE_H

#include <thread>
#include <functional>

#include "rdma_pre.h"

static char LOCALHOST[] = "localhost";

class TCP_Sock_Pre : public RDMA_Pre
{
public:
    TCP_Sock_Pre();
    ~TCP_Sock_Pre();

    RDMA_Endpoint* ptp_connect(RDMA_Session* session);
    void daemon_connect(RDMA_Session* session);
    void close_daemon();

private:
    void print_config();
    // Exchange QP info to establish the connection
    struct cm_con_data_t exchange_qp_data(struct cm_con_data_t local_con_data);
    // tcp_socket
    void sock_daemon_connect(int port, RDMA_Session* session);
    int sock_server_connect(int port);
    int sock_client_connect(const char *server_name, int port);
    int sock_recv(int sock_fd, size_t size, void *buf);
    int sock_send(int sock_fd, size_t size, const void *buf);
    int sock_sync_data(int sock_fd, int is_daemon, size_t size, const void *out_buf, void *in_buf);
    int sock_sync_ready(int sock_fd, int is_daemon);
    // Listen socket
    int listen_sock_;
    // Remote socket
    int remote_sock_;
    // Thread used to be as daemon
    std::unique_ptr<std::thread> daemon_thread_;
    bool run_;
};

#endif // !TCP_SOCK_PRE_H