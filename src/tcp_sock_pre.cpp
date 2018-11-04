/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: TCP_SOCK_PRE_CPP
************************************************ */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "tcp_sock_pre.h"
#include "rdma_util.h"

TCP_Sock_Pre::TCP_Sock_Pre()
{
    log_info("TCP_Sock_Pre Created");
}

TCP_Sock_Pre::~TCP_Sock_Pre()
{
    log_info("TCP_Sock_Pre Deleted");
}

void TCP_Sock_Pre::print_config()
{
    //log_func();
    fprintf(stdout, " ------------------------------------------------\n");
    if (config.server_name)
    fprintf(stdout, " Remote IP                    : %s\n", config.server_name);
    fprintf(stdout, " IB port                      : %u\n", config.ib_port);
    fprintf(stdout, " TCP port                     : %u\n", config.tcp_port);
    fprintf(stdout, " ------------------------------------------------\n");
}

cm_con_data_t TCP_Sock_Pre::exchange_qp_data(cm_con_data_t local_con_data)
{
    cm_con_data_t remote_con_data;

    if (sock_sync_data(remote_sock_, !config.server_name, sizeof(cm_con_data_t), &local_con_data, &remote_con_data) < 0)
    {
        log_error("failed to exchange connection data between sides");
    }

    return remote_con_data;
}

/*****************************************
* Function: tcp_sock_connect
*****************************************/
void TCP_Sock_Pre::pre_connect()
{
    print_config();

    // Client Side
    if (config.server_name)
    {
        remote_sock_ = sock_client_connect(config.server_name, config.tcp_port);
        if (remote_sock_ < 0) {
            log_error(make_string("failed to establish TCP connection to server %s, port %d", config.server_name, config.tcp_port));
            return;
        }
    } else
    // Server Side
    {
        log_ok(make_string("waiting on port %d for TCP connection", config.tcp_port));
        remote_sock_ = sock_server_connect(config.tcp_port);
        if (remote_sock_ < 0) {
            log_error(make_string("failed to establish TCP connection with client on port %d", config.tcp_port));
            return;
        }
    }
    log_ok("TCP connection was established");
}

/*****************************************
* Function: sock_server_connect
*****************************************/
int TCP_Sock_Pre::sock_server_connect(int port)
{
    struct addrinfo *res, *t;
    struct addrinfo hints = {
        .ai_flags    = AI_PASSIVE,
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };
    char *service;
    int n;
    int sockfd = -1, connfd;

    if (asprintf(&service, "%d", port) < 0) {
        log_error("asprintf failed");
        return -1;
    }

    n = getaddrinfo(NULL, service, &hints, &res);
    if (n < 0) {
        log_error(make_string("%s for port %d", gai_strerror(n), port));
        free(service);
        return -1;
    }

    for (t = res; t; t = t->ai_next) {
        sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
        if (sockfd >= 0) {
            n = 1;

            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof n);

            if (!bind(sockfd, t->ai_addr, t->ai_addrlen))
                break;
            close(sockfd);
            sockfd = -1;
        }
    }

    freeaddrinfo(res);
    free(service);

    if (sockfd < 0) {
        log_error(make_string("couldn't listen to port %d", port));
        return -1;
    }

    listen(sockfd, 1);
    connfd = accept(sockfd, NULL, 0);
    close(sockfd);
    if (connfd < 0) {
        log_error("accept() failed");
        return -1;
    }

    return connfd;
}

/*****************************************
* Function: sock_client_connect
*****************************************/
int TCP_Sock_Pre::sock_client_connect(const char *server_name, int port)
{
    struct addrinfo *res, *t;
    struct addrinfo hints = {
        .ai_flags    = AI_PASSIVE,
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };

    char *service;
    int n;
    int sockfd = -1;

    if (asprintf(&service, "%d", port) < 0) {
        log_error("asprintf failed");
        return -1;
    }

    n = getaddrinfo(server_name, service, &hints, &res);
    if (n < 0) {
        log_error(make_string("%s for %s:%d", gai_strerror(n), server_name, port));
        free(service);
        return -1;
    }

    for (t = res; t; t = t->ai_next) {
        sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
        if (sockfd >= 0) {
            if (!connect(sockfd, t->ai_addr, t->ai_addrlen))
                break;
            close(sockfd);
            sockfd = -1;
        }
    }
    freeaddrinfo(res);
    free(service);

    if (sockfd < 0) {
        log_error(make_string("couldn't connect to %s:%d", server_name, port));
        return -1;
    }

    return sockfd;
}

/*****************************************
* Function: sock_recv
*****************************************/
int TCP_Sock_Pre::sock_recv(int sock_fd, size_t size, void *buf)
{
    int rc;

    retry_after_signal:
    rc = recv(sock_fd, buf, size, MSG_WAITALL);
    if (rc != size) {
        fprintf(stderr, "recv failed: %s, rc=%d\n", strerror(errno), rc);

        if ((errno == EINTR) && (rc != 0))
            goto retry_after_signal;    /* Interrupted system call */
        if (rc)
            return rc;
        else
            return -1;
    }

    return 0;
}

/*****************************************
* Function: sock_send
*****************************************/
int TCP_Sock_Pre::sock_send(int sock_fd, size_t size, const void *buf)
{
    int rc;


    retry_after_signal:
    rc = send(sock_fd, buf, size, 0);

    if (rc != size) {
        fprintf(stderr, "send failed: %s, rc=%d\n", strerror(errno), rc);

        if ((errno == EINTR) && (rc != 0))
            goto retry_after_signal;    /* Interrupted system call */
        if (rc)
            return rc;
        else
            return -1;
    }

    return 0;
}

/*****************************************
* Function: sock_sync_data
*****************************************/
int TCP_Sock_Pre::sock_sync_data(int sock_fd, int is_daemon, size_t size, const void *out_buf, void *in_buf)
{
    int rc;

    if (is_daemon) {
        rc = sock_send(sock_fd, size, out_buf);
        if (rc)
            return rc;

        rc = sock_recv(sock_fd, size, in_buf);
        if (rc)
            return rc;
    } else {
        rc = sock_recv(sock_fd, size, in_buf);
        if (rc)
            return rc;

        rc = sock_send(sock_fd, size, out_buf);
        if (rc)
            return rc;
    }

    return 0;
}

/*****************************************
* Function: sock_sync_ready
*****************************************/
int TCP_Sock_Pre::sock_sync_ready(int sock_fd, int is_daemon)
{
    char cm_buf = 'a';
    return sock_sync_data(sock_fd, is_daemon, sizeof(cm_buf), &cm_buf, &cm_buf);
}