/* ***********************************************
MYID   : Chen Fan
LANG   : G++
PROG   : RDMA_MESSAGE_CPP
************************************************ */

#include "rdma_message.h"

std::string get_message(Message_type msgt)
{
    switch(msgt)
    {
        case RDMA_MESSAGE_ACK:
            return "RDMA_MESSAGE_ACK";
            break;
        case RDMA_MESSAGE_BUFFER_UNLOCK:
            return "RDMA_MESSAGE_BUFFER_UNLOCK";
            break;
        case RDMA_MESSAGE_WRITE_REQUEST:
            return "RDMA_MESSAGE_WRITE_REQUEST";
            break;
        case RDMA_MESSAGE_READ_REQUEST:
            return "RDMA_MESSAGE_READ_REQUEST";
            break;
        case RDMA_MESSAGE_CLOSE:
            return "RDMA_MESSAGE_CLOSE";
            break;
        case RDMA_MESSAGE_TERMINATE:
            return "RDMA_MESSAGE_TERMINATE";
            break;
        default:
            return "UNKNOWN MESSAGE";
    }
}