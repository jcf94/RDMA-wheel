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

void fill_message_content(char* target, void* addr, uint64_t size, ibv_mr* mr)
{
    memcpy(&target[kBufferSizeStartIndex], &(size), sizeof(size));
    memcpy(&target[kRemoteAddrStartIndex], &(addr), sizeof(addr));
    if (mr) memcpy(&target[kRkeyStartIndex], &(mr->rkey), sizeof(mr->rkey));
}

Message_Content parse_message_content(char* content)
{
    Message_Content msg;

    memcpy(&(msg.buffer_size_), &content[kBufferSizeStartIndex], 8);
    memcpy(&(msg.remote_addr_), &content[kRemoteAddrStartIndex], 8);
    memcpy(&(msg.rkey_), &content[kRkeyStartIndex], 4);

    return msg;
}