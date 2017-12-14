/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_MAIN
************************************************ */

#include "src/rdma_session.h"

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
        RDMA_Pre pre_tcp;

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
        pre_tcp.print_config();
        pre_tcp.tcp_sock_connect();

        RDMA_Session session;
        RDMA_Endpoint* endpoint = session.add_connection(&pre_tcp);

        if (strcmp(argv[1], "s") == 0)
        {
            
        } else if (strcmp(argv[1], "c") == 0)
        {

            endpoint->send_message(RDMA_MESSAGE_READ_REQUEST);
            endpoint->send_message(RDMA_MESSAGE_CLOSE);
            endpoint->send_message(RDMA_MESSAGE_TERMINATE);
        }

        //session.endpoint_->send_message(RDMA_MESSAGE_ACK);
        //session.endpoint_->send_message(RDMA_MESSAGE_CLOSE);
    }
    
    return 0;
}