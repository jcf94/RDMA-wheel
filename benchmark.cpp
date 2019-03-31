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

#include "src/rdma_util.h"

#include "src/rdma_session.h"
#include "src/rdma_endpoint.h"
#include "src/tcp_sock_pre.h"
#include "src/rdma_message.h"
#include "src/rdma_buffer.h"

#include "src/rdma_channel.h"

using namespace std;

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
        int total_data = 256 * MB;
        int block_data = 4 * KB;

        if (strcmp(argv[1], "s") == 0)
        {

        } else if (strcmp(argv[1], "c") == 0)
        {
            //char* test_data = (char*)malloc(total_data);
            RDMA_Buffer* test_data = endpoint->bufferalloc(total_data);
            printf("%p\n", test_data->buffer());

            // Warm Up
            // endpoint->set_sync_barrier(total_data);
            // for (int i=0;i<total_data;i+=block_data)
            // {
            //     RDMA_Buffer* temp = new RDMA_Buffer(test_data, i, block_data);
            //     //printf("%p\n", temp->buffer());
            //     endpoint->send_data(temp);
            //     //endpoint->send_rawdata((void*)(test_data+i), block_data);
            // }
            // endpoint->wait_for_sync();
            // log_ok(make_string("%d", endpoint->channel()->get_table_size()));

            log_ok("Test Start");

            std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

            endpoint->set_sync_barrier(total_data);
            for (int i=0;i<total_data;i+=block_data)
            {
                RDMA_Buffer* temp = new RDMA_Buffer(test_data, i, block_data);
                endpoint->send_data(temp);
                //endpoint->send_rawdata((void*)(test_data+i), block_data);
            }
            endpoint->wait_for_sync();

            std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

            log_ok(make_string("Send Over, Total %lld Bytes with Block size %lld Bytes", total_data, block_data));
            log_ok(make_string("Total Time used: %lld ms, Bandwidth: %lf MB/s", duration, (double)(total_data / MB) / duration * 1000));

            endpoint->close();
        }
    }
    
    return 0;
}