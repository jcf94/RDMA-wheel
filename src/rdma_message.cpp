/* ***********************************************
MYID   : Chen Fan
LANG   : G++
PROG   : RDMA_MESSAGE_CPP
************************************************ */

#include "rdma_util.h"

#include "rdma_message.h"
#include "rdma_channel.h"
#include "rdma_endpoint.h"
#include "rdma_buffer.h"

std::mutex RDMA_Message::sync_cv_mutex;
std::condition_variable RDMA_Message::sync_cv;
bool RDMA_Message::sync_flag = false;

std::string RDMA_Message::get_message(Message_type msgt)
{
    switch(msgt)
    {
        case RDMA_MESSAGE_ACK:
            return "RDMA_MESSAGE_ACK";
            break;
        case RDMA_MESSAGE_CLOSE:
            return "RDMA_MESSAGE_CLOSE";
            break;
        case RDMA_MESSAGE_TERMINATE:
            return "RDMA_MESSAGE_TERMINATE";
            break;
        case RDMA_MESSAGE_SYNC_ACK:
            return "RDMA_MESSAGE_SYNC_ACK";
            break;

        case RDMA_MESSAGE_SYNC_REQUEST:
            return "RDMA_MESSAGE_SYNC_REQUEST";
            break;
        case RDMA_MESSAGE_WRITE_REQUEST:
            return "RDMA_MESSAGE_WRITE_REQUEST";
            break;
        case RDMA_MESSAGE_WRITE_READY:
            return "RDMA_MESSAGE_WRITE_READY";
            break;
        case RDMA_MESSAGE_READ_REQUEST:
            return "RDMA_MESSAGE_READ_REQUEST";
            break;
        case RDMA_MESSAGE_READ_OVER:
            return "RDMA_MESSAGE_READ_OVER";
            break;

        case RDMA_DATA:
            return "RDMA_DATA";
            break;

        default:
            return "UNKNOWN MESSAGE";
    }
}

void RDMA_Message::fill_message_content(char* target, void* addr, uint64_t size, ibv_mr* mr)
{
    memcpy(&target[kBufferSizeStartIndex], &(size), sizeof(size));
    memcpy(&target[kRemoteAddrStartIndex], &(addr), sizeof(addr));
    if (mr) memcpy(&target[kRkeyStartIndex], &(mr->rkey), sizeof(mr->rkey));
}

Message_Content RDMA_Message::parse_message_content(char* content)
{
    Message_Content msg;

    memcpy(&(msg.buffer_size), &content[kBufferSizeStartIndex], 8);
    memcpy(&(msg.buffer_mr.remote_addr), &content[kRemoteAddrStartIndex], 8);
    memcpy(&(msg.buffer_mr.rkey), &content[kRkeyStartIndex], 4);

    return msg;
}


void RDMA_Message::send_message_to_channel(RDMA_Channel* channel, Message_type msgt, uint64_t data)
{
    switch(msgt)
    {
        case RDMA_MESSAGE_ACK:
        case RDMA_MESSAGE_CLOSE:
        case RDMA_MESSAGE_TERMINATE:
        case RDMA_MESSAGE_SYNC_ACK:
        {
            channel->send(msgt, 0);
            break;
        }
        case RDMA_MESSAGE_SYNC_REQUEST:
        {
            channel->task_with_lock([channel, msgt, data]
            {
                RDMA_Message::fill_message_content((char*)channel->outgoing()->buffer(), 0, data, NULL);
                channel->write(msgt, kBufferSizeEndIndex);
            });
            break;
        }
        case RDMA_MESSAGE_READ_OVER:
        {
            channel->task_with_lock([channel, msgt, data]
            {
                RDMA_Message::fill_message_content((char*)channel->outgoing()->buffer(), (void*)data, kRemoteAddrEndIndex, NULL);
                channel->write(msgt, kRemoteAddrEndIndex);
            });
            break;
        }
        default:
            log_error("Unsupported Message Type");
    }
}

// --------------------- Message Process ---------------------

void RDMA_Message::process_attached_message(const ibv_wc &wc)
{
    // Which RDMA_Channel get this message
    RDMA_Channel* channel = reinterpret_cast<RDMA_Channel*>(wc.wr_id);
    // Consumed a ibv_post_recv, so add one
    channel->recv();

    Message_type msgt = (Message_type)wc.imm_data;
    log_info(make_string("Message Recv: %s", RDMA_Message::get_message(msgt).data()));
    switch(msgt)
    {
        case RDMA_MESSAGE_SYNC_REQUEST:
        {
            Message_Content msg = RDMA_Message::parse_message_content((char*)channel->incoming()->buffer());
            send_message_to_channel(channel, RDMA_MESSAGE_ACK);

            channel->endpoint()->target_count_set(msg.buffer_size);
            break;
        }
        case RDMA_MESSAGE_READ_OVER:
        {
            Message_Content msg = RDMA_Message::parse_message_content((char*)channel->incoming()->buffer());
            send_message_to_channel(channel, RDMA_MESSAGE_ACK);

            RDMA_Buffer* buf = (RDMA_Buffer*)channel->find_in_table((uint64_t)msg.buffer_mr.remote_addr);
            delete buf;

            break;
        }
        case RDMA_MESSAGE_READ_REQUEST:
        {
            Message_Content msg = RDMA_Message::parse_message_content((char*)channel->incoming()->buffer());
            send_message_to_channel(channel, RDMA_MESSAGE_ACK);

            RDMA_Buffer* test_new = new RDMA_Buffer(channel, channel->pd(), msg.buffer_size);

            channel->insert_to_table((uint64_t)test_new, (uint64_t)msg.buffer_mr.remote_addr
            );

            channel->read_data(test_new, msg);

            break;
        }
        default:
            log_error("Unsupported Message Type");
    }
}

void RDMA_Message::process_immediate_message(const ibv_wc &wc, Session_status &status, std::vector<RDMA_Endpoint*> &endpoint_list)
{
    // Which RDMA_Channel get this message
    RDMA_Channel* channel = reinterpret_cast<RDMA_Channel*>(wc.wr_id);
    // Consumed a ibv_post_recv, so add one
    channel->recv();

    Message_type msgt = (Message_type)wc.imm_data;
    log_info(make_string("Message Recv: %s", RDMA_Message::get_message(msgt).data()));
    switch(msgt)
    {
        case RDMA_MESSAGE_ACK:
        {
            channel->release_remote();

            break;
        }
        case RDMA_MESSAGE_CLOSE:
        {
            for (auto endpoint: endpoint_list)
                send_message_to_channel(endpoint->channel(), RDMA_MESSAGE_TERMINATE);
            status = CLOSING;
            break;
        }
        case RDMA_MESSAGE_TERMINATE:
        {
            status = CLOSED;
            channel->endpoint()->connected_ = false;
            break;
        }
        case RDMA_MESSAGE_SYNC_ACK:
        {
            std::lock_guard<std::mutex> lock(RDMA_Message::sync_cv_mutex);
            RDMA_Message::sync_flag = true;
            RDMA_Message::sync_cv.notify_one();
            break;
        }
        default:
            log_error("Unsupported Message Type");
    }
}

void RDMA_Message::process_write_success(const ibv_wc &wc)
{
    log_info(make_string("Message Write Success"));
    // Which RDMA_Channel send this message/data
    RDMA_Channel* channel = reinterpret_cast<RDMA_Channel*>(wc.wr_id);

    // Message sent success, unlock the channel outgoing
    channel->release_local();
}

void RDMA_Message::process_send_success(const ibv_wc &wc)
{
    // Which RDMA_Channel send this message
    // RDMA_Channel* channel = reinterpret_cast<RDMA_Channel*>(wc_[i].wr_id);
    log_info(make_string("Message Send Success"));
}

void RDMA_Message::process_read_success(const ibv_wc &wc)
{
    RDMA_Buffer* rb = reinterpret_cast<RDMA_Buffer*>(wc.wr_id);
    char* temp = (char*)rb->buffer();
    //log_ok(make_string("RDMA Read: %s", temp));
    
    RDMA_Channel* channel = rb->channel();

    if (channel->endpoint()->data_recv_success(rb->size()))
    {
        log_ok("recv over");
        send_message_to_channel(channel, RDMA_MESSAGE_SYNC_ACK);
    }
    //log_ok(make_string("RDMA Read: %d bytes, total %d bytes", rb->size(), endpoint->total_recv_data()));

    uint64_t res = channel->find_in_table((uint64_t)rb);
    send_message_to_channel(channel, RDMA_MESSAGE_READ_OVER, res);

    delete rb;
}