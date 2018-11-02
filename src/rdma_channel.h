/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_CHANNEL_H
************************************************ */

#ifndef RDMA_CHANNEL_H
#define RDMA_CHANNEL_H

#include "rdma_buffer.h"

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
    // |    8B     |       8B  | 4B |
};

static const size_t kBufferSizeStartIndex = 0;
static const size_t kRemoteAddrStartIndex = kBufferSizeStartIndex + sizeof(Remote_info::buffer_size_);
static const size_t kRkeyStartIndex = kRemoteAddrStartIndex + sizeof(Remote_info::remote_addr_);
static const size_t kMessageTotalBytes = kRkeyStartIndex + sizeof(Remote_info::rkey_);

enum Channel_status
{
    NONE,
    IDLE,
    LOCK
};

class RDMA_Endpoint;

class RDMA_Channel
{
public:
    friend class TCP_Sock_Pre;
    friend class RDMA_Session;
    friend class RDMA_Endpoint;

    RDMA_Channel(RDMA_Endpoint* endpoint, ibv_pd* pd, ibv_qp* qp);
    ~RDMA_Channel();

    void send(Message_type msgt, uint64_t addr = 0);
    void write(uint32_t imm_data, size_t size, uint64_t wr_id = 0);

private:
    void ParseMessage(Message_type& rm, void* buffer);

    RDMA_Endpoint* endpoint_;
    ibv_pd* pd_;
    ibv_qp* qp_;

    RDMA_Buffer* incoming_;
    RDMA_Buffer* outgoing_;

    Remote_MR remote_mr_;

    Channel_status local_status_;
    Channel_status remote_status_;
};

#endif // !RDMA_CHANNEL_H