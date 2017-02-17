#include "ga_core.h"

/*static uint64_t msg_id;*/

extern struct string msg_strings[];

struct msg *req_get(struct conn *conn) {
    struct msg *msg;

    msg = msg_get(conn, true);

    return msg;
}

void req_put(struct context *ctx, struct msg *msg) {
    struct msg *pmsg;

    pmsg = msg->peer;
    if (pmsg != NULL) {
        msg->peer = NULL;
        pmsg->peer = NULL;
        rsp_put(ctx, pmsg);
    }

    msg_put(ctx, msg);
}

/*
 * Return true if request is done, false otherwise
 */
bool req_done(struct conn *conn, struct msg *msg) {
    if (!msg->done) {
        return false;
    }

    return true;
}

void req_enqueue_omsgq(struct context *ctx, struct conn *conn, struct msg *msg) {
    ASSERT(msg->request);
    ASSERT(!msg->noreply);

    TAILQ_INSERT_TAIL(&conn->omsg_q, msg, c_tqe);
}

struct msg *req_recv_next(struct context *ctx, struct conn *conn, bool alloc) {
    struct msg *msg;

    if (conn->eof) {
        msg = conn->rmsg;

        if (msg != NULL) {
            conn->rmsg = NULL;
            req_put(ctx, msg);
        }

        /*if (!conn->active(conn)) {*/
        /*conn->done = 1;*/
        /*}*/

        return NULL;
    }

    msg = conn->rmsg;
    if (msg != NULL) {
        return msg;
    }

    if (!alloc)
        return NULL;

    msg = req_get(conn);
    if (msg != NULL) {
        conn->rmsg = msg;
    }

    return msg;
}

static void req_process_get(struct context *ctx, struct conn *conn, struct msg *msg) {
    fdb_doc *doc;

    doc = forestdb_get(ctx, msg->key_start, msg->key_end - msg->key_start);

    if (doc == NULL) {
        rsp_send_status(ctx, conn, msg, MSG_RSP_END);
        return;
    }

    if (doc->bodylen == 0) {
        rsp_send_status(ctx, conn, msg, MSG_RSP_END);
        forestdb_doc_free(doc);
        return;
    }

    rsp_send_doc(ctx, conn, msg, doc);
    forestdb_doc_free(doc);
}

static void req_process_set(struct context *ctx, struct conn *conn, struct msg *msg) {
    int status;

    status = forestdb_set(ctx, msg->key_start, msg->key_end - msg->key_start, msg->value, msg->vlen, msg->flags, msg->expiry);

    if (status != GA_OK) {
        req_process_error(ctx, conn, msg, errno);
        return;
    }

    rsp_send_status(ctx, conn, msg, MSG_RSP_STORED);
}

static void req_process(struct context *ctx, struct conn *conn, struct msg *msg) {
    if (!msg->noreply) {
        req_enqueue_omsgq(ctx, conn, msg);
    }

    switch (msg->type) {
        case MSG_REQ_GET:
        case MSG_REQ_GETS:
            req_process_get(ctx, conn, msg);
            break;
        case MSG_REQ_SET:
            req_process_set(ctx, conn, msg);
            break;
        default:
            ASSERT(0);
    }
}

static bool req_filter(struct context *ctx, struct conn *conn, struct msg *msg) {
    if (msg_empty(msg)) {
        req_put(ctx, msg);
        return true;
    }

    return false;
}

void req_recv_done(struct context *ctx, struct conn *conn, struct msg *msg, struct msg *nmsg) {
    ASSERT(msg->request);
    ASSERT(conn->rmsg == msg);

    conn->rmsg = nmsg;

    if (req_filter(ctx, conn, msg)) {
        return;
    }

    req_process(ctx, conn, msg);
}

void req_process_error(struct context *ctx, struct conn *conn, struct msg *msg, int err) {
    int status;

    msg->done = 1;
    msg->error = 1;
    msg->err = err != 0 ? err : errno;

    ga_log(ERROR, "process req type %d on sd %d failed: %s", msg->type, conn->sd, strerror(msg->err));

    if (msg->noreply) {
        req_put(ctx, msg);
        return;
    }

    if (req_done(conn, TAILQ_FIRST(&conn->omsg_q))) {
        status = event_add_out(ctx->base->ep, conn);
        if (status != GA_OK) {
            conn->err = errno;
        }
    }
}
