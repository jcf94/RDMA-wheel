/* ***********************************************
MYID   : Chen Fan
LANG   : G++
PROG   : RDMA_MESSAGE_H
************************************************ */

#ifndef RDMA_MESSAGE_H
#define RDMA_MESSAGE_H

#include "rdma_util.h"

enum Message_type
{
    RDMA_MESSAGE_ACK,
    RDMA_MESSAGE_BUFFER_UNLOCK,
    RDMA_MESSAGE_WRITE_REQUEST,
    RDMA_MESSAGE_READ_REQUEST,
    RDMA_MESSAGE_CLOSE,
    RDMA_MESSAGE_TERMINATE
};

struct Message_Content
{
    uint64_t buffer_size_;
    uint64_t remote_addr_;
    uint32_t rkey_;

    // |buffer_size|remote_addr|rkey|
    // |    8B     |     8B    | 4B |
};

static const size_t kBufferSizeStartIndex = 0;
static const size_t kRemoteAddrStartIndex = kBufferSizeStartIndex + sizeof(Message_Content::buffer_size_);
static const size_t kRkeyStartIndex = kRemoteAddrStartIndex + sizeof(Message_Content::remote_addr_);
static const size_t kMessageTotalBytes = kRkeyStartIndex + sizeof(Message_Content::rkey_);

std::string get_message(Message_type msgt);

void fill_message_content(char* target, void* addr, uint64_t size, ibv_mr* mr);

#endif // !RDMA_MESSAGE_H