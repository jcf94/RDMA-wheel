/* ***********************************************
MYID   : Chen Fan
LANG   : G++
PROG   : RDMA_MESSAGE_H
************************************************ */

#ifndef RDMA_MESSAGE_H
#define RDMA_MESSAGE_H

#include <string>

enum Message_type
{
    RDMA_MESSAGE_ACK,
    RDMA_MESSAGE_BUFFER_UNLOCK,
    RDMA_MESSAGE_WRITE_REQUEST,
    RDMA_MESSAGE_READ_REQUEST,
    RDMA_MESSAGE_CLOSE,
    RDMA_MESSAGE_TERMINATE
};

std::string get_message(Message_type msgt);

struct Remote_info
{
    uint64_t buffer_size_;
    uint64_t remote_addr_;
    uint32_t rkey_;

    // |buffer_size|remote_addr|rkey|
    // |    8B     |     8B    | 4B |
};

static const size_t kBufferSizeStartIndex = 0;
static const size_t kRemoteAddrStartIndex = kBufferSizeStartIndex + sizeof(Remote_info::buffer_size_);
static const size_t kRkeyStartIndex = kRemoteAddrStartIndex + sizeof(Remote_info::remote_addr_);
static const size_t kMessageTotalBytes = kRkeyStartIndex + sizeof(Remote_info::rkey_);

#endif // !RDMA_MESSAGE_H