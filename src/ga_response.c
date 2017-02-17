#include "ga_core.h"

extern struct string msg_strings[];

struct msg *rsp_get(struct conn *conn) {
    struct msg *msg;

    msg = msg_get(conn, false);
    if (msg == NULL)
        conn->err = errno;

    return msg;
}

void req_dequeue_omsgq(struct context *ctx, struct conn *conn, struct msg *msg) {
    ASSERT(msg->request);
    ASSERT(!msg->noreply);

    TAILQ_REMOVE(&conn->omsg_q, msg, c_tqe);
}

void rsp_put(struct context *ctx, struct msg *msg) { msg_put(ctx, msg); }

void rsp_send_doc(struct context *ctx, struct conn *conn, struct msg *msg, fdb_doc *doc) {
    int status;
    struct msg *pmsg;
    struct string *str;
    char num[GA_UINTMAX_MAXLEN];
    size_t n;

    pmsg = rsp_get(conn);

    /* copy "VALUE "*/
    str = &msg_strings[MSG_RSP_VALUE];
    status = mbuf_copy_from(ctx, &pmsg->mhdr, str->data, str->len);
    if (status != GA_OK) {
        req_process_error(ctx, conn, msg, errno);
        return;
    }
    pmsg->mlen += str->len;

    /* copy key */
    status = mbuf_copy_from(ctx, &pmsg->mhdr, msg->key_start, msg->key_end - msg->key_start);
    if (status != GA_OK) {
        req_process_error(ctx, conn, msg, errno);
        return;
    }
    pmsg->mlen += msg->key_end - msg->key_start;

    /* copy flags */
    uint8_t *meta = doc->meta;
    char *flags = strtok((char *)meta, ":");

    mbuf_copy_from(ctx, &pmsg->mhdr, (uint8_t *)" ", 1);
    if (status != GA_OK) {
        req_process_error(ctx, conn, msg, errno);
        return;
    }
    status = mbuf_copy_from(ctx, &pmsg->mhdr, (uint8_t *)flags, strlen(flags));
    if (status != GA_OK) {
        req_process_error(ctx, conn, msg, errno);
        return;
    }
    pmsg->mlen += strlen(flags);

    /* copy value length */
    n = sprintf(num, " %d", (int)doc->bodylen);
    status = mbuf_copy_from(ctx, &pmsg->mhdr, (uint8_t *)num, n);
    if (status != GA_OK) {
        req_process_error(ctx, conn, msg, errno);
        return;
    }
    pmsg->mlen += doc->bodylen;

    /* copy CRLF */
    str = &msg_strings[MSG_CRLF];
    status = mbuf_copy_from(ctx, &pmsg->mhdr, str->data, str->len);
    if (status != GA_OK) {
        req_process_error(ctx, conn, msg, errno);
        return;
    }
    pmsg->mlen += str->len;

    /* copy value */
    status = mbuf_copy_from(ctx, &pmsg->mhdr, doc->body, doc->bodylen);
    if (status != GA_OK) {
        req_process_error(ctx, conn, msg, errno);
        return;
    }
    pmsg->mlen += str->len;

    /* copy CRLF */
    str = &msg_strings[MSG_CRLF];
    status = mbuf_copy_from(ctx, &pmsg->mhdr, str->data, str->len);
    if (status != GA_OK) {
        req_process_error(ctx, conn, msg, errno);
        return;
    }
    pmsg->mlen += str->len;

    /* copy END */
    if (msg->frag_id == 0 || msg->last_fragment) {
        str = &msg_strings[MSG_RSP_END];
        status = mbuf_copy_from(ctx, &pmsg->mhdr, str->data, str->len);
        if (status != GA_OK) {
            req_process_error(ctx, conn, msg, errno);
            return;
        }
        pmsg->mlen += str->len;
    }

    msg->done = 1;
    msg->peer = pmsg;
    pmsg->peer = msg;

    status = event_add_out(ctx->base->ep, conn);
    if (status != GA_OK) {
        ga_log(ERROR, "add out error");
        return;
    }
}

struct msg *rsp_send_next(struct context *ctx, struct conn *conn) {
    int status;
    struct msg *msg, *pmsg;

    pmsg = TAILQ_FIRST(&conn->omsg_q);
    if (pmsg == NULL || !pmsg->done) {
        if (pmsg == NULL && conn->eof) {
            conn->done = 1;
            ga_log(DEBUG, "c %d is done", conn->sd);
        }

        status = event_del_out(ctx->base->ep, conn);

        if (status != GA_OK)
            conn->err = errno;

        return NULL;
    }

    msg = conn->smsg;
    if (msg != NULL) {
        pmsg = TAILQ_NEXT(msg->peer, c_tqe);
    }

    if (pmsg == NULL || !pmsg->done) {
        conn->smsg = NULL;
        return NULL;
    }

    msg = pmsg->peer;
    conn->smsg = msg;

    return msg;
}

void rsp_send_done(struct context *ctx, struct conn *conn, struct msg *msg) {
    struct msg *pmsg; /* peer message (request) */

    ASSERT(conn->smsg == NULL);

    ga_log(DEBUG, "send done rsp %" PRIu64 " on c %d", msg->id, conn->sd);

    pmsg = msg->peer;

    req_dequeue_omsgq(ctx, conn, pmsg);

    req_put(ctx, pmsg);
}

void rsp_send_status(struct context *ctx, struct conn *conn, struct msg *msg, msg_type_t rsp_type) {
    int status;
    struct msg *pmsg;
    struct string *str;

    if (msg->noreply) {
        req_put(ctx, msg);
        return;
    }

    pmsg = rsp_get(conn);
    if (pmsg == NULL) {
        req_process_error(ctx, conn, msg, ENOMEM);
        return;
    }

    str = &msg_strings[rsp_type];
    status = mbuf_copy_from(ctx, &pmsg->mhdr, str->data, str->len);
    if (status != GA_OK) {
        req_process_error(ctx, conn, msg, errno);
        return;
    }
    pmsg->mlen += str->len;

    msg->done = 1;
    msg->peer = pmsg;
    pmsg->peer = msg;

    status = event_add_out(ctx->base->ep, conn);
    if (status != GA_OK) {
        req_process_error(ctx, conn, msg, errno);
        return;
    }
}
