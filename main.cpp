/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_MAIN
************************************************ */

#include "src/rdma_util.h"

#include "src/rdma_session.h"
#include "src/rdma_endpoint.h"
#include "src/tcp_sock_pre.h"

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
        //RDMA_Endpoint* endpoint = session.ptp_connect(&pre_tcp);

        if (strcmp(argv[1], "s") == 0)
        {
            session.daemon_connect(&pre_tcp);

            // RDMA_Endpoint* endpoint = pre_tcp.ptp_connect(&session);
            // char a[] = "Hello World from Server !!!!!";
            // endpoint->send_data((void*)a, sizeof(a));

        } else if (strcmp(argv[1], "c") == 0)
        {
            RDMA_Endpoint* endpoint = session.ptp_connect(&pre_tcp);

            char a[] = "Hello World from Client !!!!!";
            endpoint->send_data((void*)a, sizeof(a));

            char b[] = "Test Test Test Test Test Test Test Test Test";

            while (1)
            {
                int com;
                std::cin >> com;
                if (com == 0) endpoint->send_data((void*)b, sizeof(b));
                else
                {
                    endpoint->close();
                    break;
                }
            }
        }
    }

    return 0;
}