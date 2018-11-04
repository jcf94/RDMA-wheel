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

        RDMA_Session session;
        RDMA_Endpoint* endpoint = pre_tcp.ptp_connect(&session);

        if (strcmp(argv[1], "s") == 0)
        {
            //pre_tcp.daemon_connect(&session);
            //RDMA_Endpoint* endpoint = pre_tcp.ptp_connect(&session);

            char a[] = "Hello World from Server !!!!!";
            endpoint->send_data((void*)a, sizeof(a));

        } else if (strcmp(argv[1], "c") == 0)
        {
            //RDMA_Endpoint* endpoint = pre_tcp.ptp_connect(&session);

            char a[] = "Hello World from Client !!!!!";
            endpoint->send_data((void*)a, sizeof(a));

            char b[] = "Test Test Test Test Test Test Test Test Test";
            for (int i=0;i<5;i++)
            endpoint->send_data((void*)b, sizeof(b));

            std::this_thread::sleep_for(std::chrono::seconds(2));

            endpoint->close();
        }
    }

    return 0;
}