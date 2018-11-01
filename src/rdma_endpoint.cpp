/* ***********************************************
MYID	: Chen Fan
LANG	: G++
PROG	: RDMA_ENDPOINT_CPP
************************************************ */

#include "rdma_endpoint.h"

RDMA_Endpoint::RDMA_Endpoint(RDMA_Session* session, int ib_port)
    : session_(session), ib_port_(ib_port), connected_(false)
{
    // create the Queue Pair
    struct ibv_qp_init_attr qp_init_attr;
    memset(&qp_init_attr, 0, sizeof(qp_init_attr));

    qp_init_attr.qp_type    = IBV_QPT_RC;
    // qp_init_attr.sq_sig_all = 1;
    qp_init_attr.send_cq    = session_->cq_;
    qp_init_attr.recv_cq    = session_->cq_;
    qp_init_attr.cap.max_send_wr  = RDMA_Session::CQ_SIZE;
    qp_init_attr.cap.max_recv_wr  = RDMA_Session::CQ_SIZE;
    qp_init_attr.cap.max_send_sge = 1;
    qp_init_attr.cap.max_recv_sge = 1;
    
    qp_ = ibv_create_qp(session_->pd_, &qp_init_attr);
    if (!qp_)
    {
        log_error("failed to create QP");
        // return 1;
    }
    
    log_info(make_string("QP was created, QP number=0x%x\n", qp_->qp_num));

    modify_qp_to_init();

    // Local address
    struct ibv_port_attr attr;
    if (ibv_query_port(session_->context_, ib_port_, &attr))
    {
        log_error(make_string("ibv_query_port on port %u failed", ib_port_));
    }
    self_.lid = attr.lid;
    self_.qpn = qp_->qp_num;
    self_.psn = static_cast<uint32_t>(New64()) & 0xffffff;

    // int SIZE = 32;
    // // Create Message Buffer ......
    // incoming_mbuffer_ = new RDMA_Buffer(this, SIZE);
    // outgoing_mbuffer_ = new RDMA_Buffer(this, SIZE);
    // // ACK Buffer ......
    // incoming_abuffer_ = new RDMA_Buffer(this, SIZE);
    // outgoing_abuffer_ = new RDMA_Buffer(this, SIZE);
    message_ = new RDMA_Message(session_->pd_, qp_);

    // post recv
    for (int i=0;i<100;i++)
    {
        recv();
    }

    log_info("RDMA_Endpoint Created");
}

RDMA_Endpoint::~RDMA_Endpoint()
{
    delete message_;

    if (ibv_destroy_qp(qp_))
    {
        log_error("Failed to destroy QP");
    }

    log_info("RDMA_Endpoint Deleted");
}

void RDMA_Endpoint::connect()
{
    if (connected_)
    {
        log_info("Channel Already Connected.");
    } else
    {
        modify_qp_to_rtr();
        modify_qp_to_rts();

        connected_ = true;
    }
}

void RDMA_Endpoint::recv()
{
    struct ibv_recv_wr wr;
    memset(&wr, 0, sizeof(wr));
    wr.wr_id = (uint64_t) this; // Which RDMA_Endpoint get this message
    struct ibv_recv_wr* bad_wr;
    if (ibv_post_recv(qp_, &wr, &bad_wr))
    {
        log_error("Failed to post recv");
    }
}

void RDMA_Endpoint::send_message(Message_type msgt)
{
    message_->send(msgt);
}

void RDMA_Endpoint::read_data(RDMA_Buffer* buffer, Remote_info msg)
{
    struct ibv_sge list;
    list.addr = (uint64_t) buffer->buffer_;
    list.length = msg.buffer_size_;
    list.lkey = buffer->mr_->lkey;

    struct ibv_send_wr wr;
    memset(&wr, 0, sizeof(wr));
    wr.wr_id = (uint64_t) buffer;
    wr.sg_list = &list;
    wr.num_sge = 1;
    wr.opcode = IBV_WR_RDMA_READ;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = (uint64_t) msg.remote_addr_;
    wr.wr.rdma.rkey = msg.rkey_;

    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(qp_, &wr, &bad_wr))
    {
        log_error("Failed to post send");
    }
}

int RDMA_Endpoint::modify_qp_to_init()
{
    struct ibv_qp_attr attr;
    int flags;
    int rc;

    // do the following QP transition: RESET -> INIT
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = ib_port_;
    attr.pkey_index = 0;
    attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ;

    flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;

    rc = ibv_modify_qp(qp_, &attr, flags);
    if (rc) {
        fprintf(stderr, "failed to modify QP state to INIT\n");
        return rc;
    }

    return 0;
}

int RDMA_Endpoint::modify_qp_to_rtr()
{
    struct ibv_qp_attr attr;
    int flags;
    int rc;

    // do the following QP transition: INIT -> RTR
    memset(&attr, 0, sizeof(attr));

    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_4096;
    attr.dest_qp_num = remote_.qpn;
    attr.rq_psn = remote_.psn;
    attr.max_dest_rd_atomic = 1;
    attr.min_rnr_timer = 0x12;
    attr.ah_attr.is_global = 0;
    attr.ah_attr.dlid = remote_.lid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = ib_port_;

    flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | 
        IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;

    rc = ibv_modify_qp(qp_, &attr, flags);
    if (rc) {
        fprintf(stderr, "failed to modify QP state to RTR\n");
        return rc;
    }

    return 0;
}

int RDMA_Endpoint::modify_qp_to_rts()
{
    struct ibv_qp_attr attr;
    int flags;
    int rc;

    // do the following QP transition: RTR -> RTS
    memset(&attr, 0, sizeof(attr));

    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = 0x14;
    attr.retry_cnt = 7;
    attr.rnr_retry = 7; // infinite
    attr.sq_psn = self_.psn;
    attr.max_rd_atomic = 1;

    flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | 
        IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

    rc = ibv_modify_qp(qp_, &attr, flags);
    if (rc) {
        fprintf(stderr, "failed to modify QP state to RTS\n");
        return rc;
    }

    return 0;
}

