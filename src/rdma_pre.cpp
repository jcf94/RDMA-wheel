/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_PRE_CPP
************************************************ */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "rdma_pre.h"

// structure to exchange data which is needed to connect the QPs
struct cm_con_data_t {
    uint64_t    maddr;      // Buffer address
    uint32_t    mrkey;      // Remote key
    uint32_t    qpn;        // QP number
    uint32_t    lid;        // LID of the IB port
    uint32_t    psn;
} __attribute__ ((packed));

RDMA_Pre::RDMA_Pre()
{
    log_info("RDMA_Pre Created");
}

RDMA_Pre::~RDMA_Pre()
{
    log_info("RDMA_Pre Deleted");
}

void RDMA_Pre::print_config()
{
    log_func();
    fprintf(stdout, " ------------------------------------------------\n");
    if (config.dev_name) fprintf(stdout, " Device name                  : \"%s\"\n", config.dev_name);
    else fprintf(stdout, " Device name                  : No Default Device\n");
    fprintf(stdout, " IB port                      : %u\n", config.ib_port);
    if (config.server_name)
        fprintf(stdout, " IP                           : %s\n", config.server_name);
    fprintf(stdout, " TCP port                     : %u\n", config.tcp_port);
    fprintf(stdout, " ------------------------------------------------\n\n");
}

void RDMA_Pre::tcp_sock_connect()
{
    if (config.server_name)
	{
		remote_sock_ = sock_client_connect(config.server_name, config.tcp_port);
		if (remote_sock_ < 0) {
			log_error(make_string("failed to establish TCP connection to server %s, port %d", config.server_name, config.tcp_port));
			return;
		}
	} else 
	{
		log_ok(make_string("waiting on port %d for TCP connection\n", config.tcp_port));
		remote_sock_ = sock_daemon_connect(config.tcp_port);
		if (remote_sock_ < 0) {
			log_error(make_string("failed to establish TCP connection with client on port %d", config.tcp_port));
			return;
		}
	}
	log_ok("TCP connection was established");
}

int RDMA_Pre::tcp_endpoint_connect(RDMA_Endpoint* endpoint)
{
    struct cm_con_data_t local_con_data, tmp_con_data;
    int rc;
    
    // exchange using TCP sockets info required to connect QPs
    local_con_data.maddr = htonll((uintptr_t)endpoint->message_->incoming_->buffer_);
    local_con_data.mrkey = htonl(endpoint->message_->incoming_->mr_->rkey);
    local_con_data.qpn = htonl(endpoint->self_.qpn);
    local_con_data.lid = htonl(endpoint->self_.lid);
    local_con_data.psn = htonl(endpoint->self_.psn);

    log_info(make_string("Local QP number  = 0x%x", endpoint->self_.qpn));
    log_info(make_string("Local LID        = 0x%x", endpoint->self_.lid));
    log_info(make_string("Local PSN        = 0x%x", endpoint->self_.psn));

    if (sock_sync_data(remote_sock_, !config.server_name, sizeof(struct cm_con_data_t), &local_con_data, &tmp_con_data) < 0)
    {
        log_error("failed to exchange connection data between sides");
        return 1;
    }

    endpoint->message_->remote_mr_.remote_addr = ntohll(tmp_con_data.maddr);
    endpoint->message_->remote_mr_.rkey = ntohl(tmp_con_data.mrkey);
    endpoint->remote_.qpn = ntohl(tmp_con_data.qpn);
    endpoint->remote_.lid = ntohl(tmp_con_data.lid);
    endpoint->remote_.psn = ntohl(tmp_con_data.psn);

    /* save the remote side attributes, we will need it for the post SR */

    //fprintf(stdout, "Remote address   = 0x%"PRIx64"\n", remote_con_data.addr);
    //fprintf(stdout, "Remote rkey      = 0x%x\n", remote_con_data.rkey);
    log_info(make_string("Remote QP number = 0x%x", endpoint->remote_.qpn));
    log_info(make_string("Remote LID       = 0x%x", endpoint->remote_.lid));
    log_info(make_string("Remote PSN       = 0x%x", endpoint->remote_.psn));
}

/*****************************************
* Function: sock_daemon_connect
*****************************************/
int RDMA_Pre::sock_daemon_connect(int port)
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
int RDMA_Pre::sock_client_connect(const char *server_name, int port)
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
int RDMA_Pre::sock_recv(int sock_fd, size_t size, void *buf)
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
int RDMA_Pre::sock_send(int sock_fd, size_t size, const void *buf)
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
int RDMA_Pre::sock_sync_data(int sock_fd, int is_daemon, size_t size, const void *out_buf, void *in_buf)
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
int RDMA_Pre::sock_sync_ready(int sock_fd, int is_daemon)
{
	char cm_buf = 'a';
	return sock_sync_data(sock_fd, is_daemon, sizeof(cm_buf), &cm_buf, &cm_buf);
}