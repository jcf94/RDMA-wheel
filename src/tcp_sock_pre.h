/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: TCP_SOCK_PRE_H
************************************************ */

#ifndef TCP_SOCK_PRE_H
#define TCP_SOCK_PRE_H

#include "rdma_pre.h"

static char LOCALHOST[] = "localhost";

class TCP_Sock_Pre : public RDMA_Pre
{
public:
    TCP_Sock_Pre();
    ~TCP_Sock_Pre();

    void pre_connect();
    struct cm_con_data_t exchange_qp_data(struct cm_con_data_t local_con_data);

private:
    void print_config();

    // tcp_socket
    int sock_daemon_connect(int port);
    int sock_client_connect(const char *server_name, int port);
    int sock_recv(int sock_fd, size_t size, void *buf);
    int sock_send(int sock_fd, size_t size, const void *buf);
    int sock_sync_data(int sock_fd, int is_daemon, size_t size, const void *out_buf, void *in_buf);
    int sock_sync_ready(int sock_fd, int is_daemon);

    int remote_sock_;
};

#endif // !TCP_SOCK_PRE_H