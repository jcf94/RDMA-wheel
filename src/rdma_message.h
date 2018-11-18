/* ***********************************************
MYID   : Chen Fan
LANG   : G++
PROG   : RDMA_MESSAGE_H
************************************************ */

#ifndef RDMA_MESSAGE_H
#define RDMA_MESSAGE_H

#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include <infiniband/verbs.h>

enum Message_type                   // Use Immediate Number as message type
{
    RDMA_MESSAGE_ACK,               // no extra data
    RDMA_MESSAGE_CLOSE,             // no extra data
    RDMA_MESSAGE_TERMINATE,         // no extra data

    RDMA_MESSAGE_WRITE_REQUEST,     // addr(map key), size
    RDMA_MESSAGE_WRITE_READY,       // addr, size, rkey
    RDMA_MESSAGE_READ_REQUEST,      // addr, size, rkey
    RDMA_MESSAGE_READ_OVER,         // addr

    RDMA_DATA,                      // not a message, but memory data

    MESSAGE_BENCHMARK_START,        // Application message
    MESSAGE_BENCHMARK_FINISH
};

struct Buffer_MR
{
    uint64_t remote_addr;
    uint32_t rkey;
};

struct Message_Content
{
    Buffer_MR buffer_mr;
    uint64_t buffer_size;

    // |remote_addr|rkey|buffer_size|
    // |     8B    | 4B |     8B    |
};

static const size_t kRemoteAddrStartIndex = 0;
static const size_t kRemoteAddrEndIndex = kRemoteAddrStartIndex + sizeof(Message_Content::buffer_mr.remote_addr);
static const size_t kRkeyStartIndex = kRemoteAddrEndIndex;
static const size_t kRkeyEndIndex = kRkeyStartIndex + sizeof(Message_Content::buffer_mr.rkey);
static const size_t kBufferSizeStartIndex = kRkeyEndIndex;
static const size_t kBufferSizeEndIndex = kBufferSizeStartIndex + sizeof(Message_Content::buffer_size);

static const size_t kMessageTotalBytes = kBufferSizeEndIndex;

enum Session_status
{
    WORK,
    CLOSING,
    CLOSED
};

class RDMA_Endpoint;
class RDMA_Channel;

namespace RDMA_Message  // Custom message protocol
{

std::string get_message(Message_type msgt);

void fill_message_content(char* target, void* addr, uint64_t size, ibv_mr* mr);
Message_Content parse_message_content(char* content);
void send_message_to_channel(RDMA_Channel* channel, Message_type msgt, uint64_t addr = 0);

void process_attached_message(const ibv_wc &wc);
void process_immediate_message(const ibv_wc &wc, Session_status &status, std::vector<RDMA_Endpoint*> &endpoint_list);
void process_write_success(const ibv_wc &wc);
void process_send_success(const ibv_wc &wc);
void process_read_success(const ibv_wc &wc);

//TODO: It's not good to use global variable like this
extern std::mutex sync_cv_mutex;
extern std::condition_variable sync_cv;
extern bool sync_flag;

};

#endif // !RDMA_MESSAGE_H