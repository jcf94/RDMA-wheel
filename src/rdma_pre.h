/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_PRE_H
************************************************ */

#ifndef RDMA_PRE_H
#define RDMA_PRE_H

#include "rdma_util.h"
#include "rdma_endpoint.h"

static char LOCALHOST[] = "localhost";

/* structure of test parameters */
struct config_t
{
    char *server_name;  /* daemon host name */
    uint32_t tcp_port;  /* daemon TCP port */
    int ib_port;        /* local IB port to work with */
};

class RDMA_Endpoint;

class RDMA_Pre
{
public:
    RDMA_Pre();
    ~RDMA_Pre();

    void print_config();
    void tcp_sock_connect();
    int tcp_endpoint_connect(RDMA_Endpoint* endpoint);

    config_t config = {
        NULL,               // server_name
        23333,              // tcp_port
        1                   // ib_port
    };

private:
    // tcp_socket
    int sock_daemon_connect(int port);
    int sock_client_connect(const char *server_name, int port);
    int sock_recv(int sock_fd, size_t size, void *buf);
    int sock_send(int sock_fd, size_t size, const void *buf);
    int sock_sync_data(int sock_fd, int is_daemon, size_t size, const void *out_buf, void *in_buf);
    int sock_sync_ready(int sock_fd, int is_daemon);

    int remote_sock_;

};

#endif // !RDMA_PRE_H