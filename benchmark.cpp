/* ***********************************************
MYID   : Chen Fan
LANG   : G++
PROG   : benchmark
************************************************ */

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <thread>
#include <chrono>

#include "src/rdma_session.h"
#include "src/rdma_endpoint.h"
#include "src/tcp_sock_pre.h"

using namespace std;

#define KB 1024
#define MB 1024 * KB
#define GB 1024 * MB

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
        RDMA_Endpoint* endpoint = session.ptp_connect(&pre_tcp);

        // Prepare data
        int total_data = 128 * MB;
        int block_data = 64 * KB;

        if (strcmp(argv[1], "s") == 0)
        {
            while (endpoint->total_recv_data() < total_data)
            {
                //log_ok(endpoint->total_recv_data());
                this_thread::sleep_for(10ms);
            }
            log_ok("recv over");
            endpoint->close();
        } else if (strcmp(argv[1], "c") == 0)
        {
            char* test_data = (char*)malloc(total_data);
            for (int i=0;i<total_data;i+=block_data)
            {
                endpoint->send_data((void*)(test_data+i), block_data);
            }
        }
    }
    
    return 0;
}