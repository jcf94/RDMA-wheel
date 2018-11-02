/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_MAIN
************************************************ */

#include "src/rdma_session.h"
#include "src/rdma_endpoint.h"
#include "src/tcp_sock_pre.h"
#include <chrono>
#include <thread>

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        log_error("Missing Start Up Config");
        log_error("Usage:");
        log_error("xxx s\tto start the server");
        log_error("xxx c\tto start the client");
    } else
    {
        RDMA_Session session;

        TCP_Sock_Pre pre_tcp;

        // Connect
        if (strcmp(argv[1], "s") == 0)
        {
            log_ok("Server Start");
        } else if (strcmp(argv[1], "c") == 0)
        {
            log_ok("Client Start");
            if (argc == 2) pre_tcp.config.server_name = LOCALHOST;
            else pre_tcp.config.server_name = argv[2];
        }

        pre_tcp.pre_connect();

        RDMA_Endpoint* endpoint = session.add_connection(&pre_tcp);

        if (strcmp(argv[1], "s") == 0)
        {
            
        } else if (strcmp(argv[1], "c") == 0)
        {
            endpoint->send_message(RDMA_MESSAGE_READ_REQUEST);

            std::this_thread::sleep_for(std::chrono::seconds(2));

            endpoint->send_message(RDMA_MESSAGE_CLOSE);
            endpoint->send_message(RDMA_MESSAGE_TERMINATE);
        }
    }
    
    return 0;
}