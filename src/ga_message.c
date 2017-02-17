#include "ga_core.h"

#define GA_IOV_MAX 128

#define DEFINE_ACTION(_hash, _name) string(_name),
struct string msg_strings[] = {MSG_CODEC(DEFINE_ACTION) null_string};
#undef DEFINE_ACTION

bool msg_empty(struct msg *msg) { return msg->mlen == 0 ? true : false; }

struct msg *msg_get(struct conn *conn, bool request) {
    struct msg *msg;
    struct context *ctx = conn->owner;

    if (!TAILQ_EMPTY(&ctx->free_msgq)) {
        msg = TAILQ_FIRST(&ctx->free_msgq);
        ctx->nfree_msgq--;
        TAILQ_REMOVE(&ctx->free_msgq, msg, m_tqe);
        goto done;
    }

    msg = ga_malloc(sizeof(struct msg));
    if (msg == NULL)
        return NULL;
done:
    STAILQ_INIT(&msg->mhdr);

    msg->parser = memcache_parse_req;

    msg->owner = conn;
    msg->mlen = 0;
    msg->state = 0;
    msg->pos = NULL;
    msg->token = NULL;
    msg->result = MSG_PARSE_OK;
    msg->type = MSG_UNKNOWN;
    msg->key_start = NULL;
    msg->key_end = NULL;
    msg->flags = 0;
    msg->expiry = 0;
    msg->vlen = 0;
    msg->rvlen = 0;
    msg->value = NULL;
    msg->cas = 0;
    msg->num = 0;

    msg->nfrag = 0;
    msg->frag_id = 0;

    msg->quit = 0;
    msg->done = 0;
    msg->err = 0;
    msg->error = 0;
    msg->request = request ? 1 : 0;
    msg->noreply = 0;

    return msg;
}

void msg_put(struct context *ctx, struct msg *msg) {
    while (!STAILQ_EMPTY(&msg->mhdr)) {
        struct mbuf *mbuf = STAILQ_FIRST(&msg->mhdr);
        mbuf_remove(&msg->mhdr, mbuf);
        mbuf_put(ctx, mbuf);
    }

    ctx->nfree_msgq++;
    TAILQ_INSERT_HEAD(&ctx->free_msgq, msg, m_tqe);
}

static int msg_parsed(struct context *ctx, struct conn *conn, struct msg *msg) {
    struct msg *nmsg;
    struct mbuf *mbuf, *nbuf;

    mbuf = STAILQ_LAST(&msg->mhdr, mbuf, next);
    if (msg->pos == mbuf->last) {
        req_recv_done(ctx, conn, msg, NULL);
        return GA_OK;
    }

    nbuf = mbuf_split(ctx, &msg->mhdr, msg->pos, NULL, NULL);
    if (nbuf == NULL)
        return GA_ENOMEM;

    nmsg = msg_get(msg->owner, msg->request);
    if (nmsg == NULL) {
        mbuf_put(ctx, nbuf);
        return GA_ENOMEM;
    }
    mbuf_insert(&nmsg->mhdr, nbuf);
    nmsg->pos = nbuf->pos;

    /* update length of current (msg) and new message (nmsg) */
    nmsg->mlen = mbuf_length(nbuf);
    msg->mlen -= nmsg->mlen;
    req_recv_done(ctx, conn, msg, NULL);

    return GA_OK;
}

static int msg_parse(struct context *ctx, struct conn *conn, struct msg *msg) {
    int status;

    if (msg_empty(msg)) {
        req_recv_done(ctx, conn, msg, NULL);
        return GA_OK;
    }

    msg->parser(msg);
    switch (msg->result) {
        case MSG_PARSE_OK:
            status = msg_parsed(ctx, conn, msg);
            break;

        case MSG_PARSE_AGAIN:
            status = GA_OK;
            break;

        default:
            status = GA_ERR;
            conn->err = errno;
            break;
    }

    return conn->err != 0 ? GA_ERR : status;
}

static int msg_recv_chain(struct context *ctx, struct conn *conn, struct msg *msg) {
    int status;
    size_t msize;
    ssize_t n;
    struct msg *nmsg;
    struct mbuf *mbuf;

    mbuf = STAILQ_LAST(&msg->mhdr, mbuf, next);
    if (mbuf == NULL || mbuf_full(mbuf)) {
        mbuf = mbuf_get(ctx);
        if (mbuf == NULL)
            return GA_ENOMEM;
        mbuf_insert(&msg->mhdr, mbuf);
        msg->pos = mbuf->pos;
    }
    ASSERT(mbuf->end - mbuf->last > 0);

    msize = mbuf_size(mbuf);

    n = conn_recv(conn, mbuf->last, msize);

    if (n < 0) {
        return n == GA_EAGAIN ? GA_OK : GA_ERR;
    }

    if (n == 0) {
        conn->done = 1;
        ga_log(DEBUG, "c %d is done", conn->sd);
        return GA_OK;
    }

    mbuf->last += n;
    msg->mlen += (uint32_t)n;

    for (;;) {
        status = msg_parse(ctx, conn, msg);
        if (status != GA_OK) {
            return status;
        }

        nmsg = req_recv_next(ctx, conn, false);

        if (nmsg == NULL || nmsg == msg)
            break;

        msg = nmsg;
    }

    return GA_OK;
}

int msg_recv(struct context *ctx, struct conn *conn) {
    struct msg *msg;
    int status;

    conn->recv_ready = 1;
    do {
        // recv
        msg = req_recv_next(ctx, conn, true);
        if (msg == NULL)
            return GA_OK;

        status = msg_recv_chain(ctx, conn, msg);

        if (status != GA_OK) {
            return status;
        }

    } while (conn->recv_ready);

    return GA_OK;
}

static int msg_send_chain(struct context *ctx, struct conn *conn, struct msg *msg) {
    struct msg_tqh send_msgq;
    struct msg *nmsg;
    struct mbuf *mbuf, *nbuf;
    size_t mlen;
    struct iovec *ciov, iov[GA_IOV_MAX];
    struct array sendv;
    size_t nsend, nsent;
    size_t limit;
    ssize_t n;

    TAILQ_INIT(&send_msgq);
    array_set(&sendv, iov, sizeof(iov[0]), GA_IOV_MAX);

    nsend = 0;
    limit = SSIZE_MAX;

    for (;;) {
        ASSERT(conn->smsg == msg);

        TAILQ_INSERT_TAIL(&send_msgq, msg, m_tqe);

        for (mbuf = STAILQ_FIRST(&msg->mhdr); mbuf != NULL && array_n(&sendv) < GA_IOV_MAX && nsend < limit; mbuf = nbuf) {
            nbuf = STAILQ_NEXT(mbuf, next);

            if (mbuf_empty(mbuf))
                continue;

            mlen = mbuf_length(mbuf);
            if ((nsend + mlen) > limit) {
                mlen = limit - nsend;
            }

            ciov = array_push(&sendv);
            ciov->iov_base = mbuf->pos;
            ciov->iov_len = mlen;

            nsend += mlen;
        }

        if (array_n(&sendv) >= GA_IOV_MAX || nsend >= limit)
            break;

        msg = rsp_send_next(ctx, conn);
        if (msg == NULL)
            break;
    }

    conn->smsg = NULL;

    if (nsend != 0) {
        n = conn_sendv(conn, &sendv, nsend);
    } else {
        ASSERT(0);
        n = 0;
    }
    nsent = n > 0 ? (size_t)n : 0;

    for (msg = TAILQ_FIRST(&send_msgq); msg != NULL; msg = nmsg) {
        nmsg = TAILQ_NEXT(msg, m_tqe);
        TAILQ_REMOVE(&send_msgq, msg, m_tqe);

        if (nsent == 0) {
            if (msg->mlen == 0) {
                rsp_send_done(ctx, conn, msg);
            }
            continue;
        }

        for (mbuf = STAILQ_FIRST(&msg->mhdr); mbuf != NULL; mbuf = nbuf) {
            nbuf = STAILQ_NEXT(mbuf, next);

            if (mbuf_empty(mbuf))
                continue;

            mlen = mbuf_length(mbuf);
            if (nsent < mlen) {
                mbuf->pos += nsent;
                nsent = 0;
                break;
            }

            mbuf->pos = mbuf->last;
            nsent -= mlen;
        }

        if (mbuf == NULL) {
            rsp_send_done(ctx, conn, msg);
        }
    }

    ASSERT(TAILQ_EMPTY(&send_msgq));

    if (n >= 0)
        return GA_OK;

    return n == GA_EAGAIN ? GA_OK : GA_ERR;
}

int msg_send(struct context *ctx, struct conn *conn) {
    struct msg *msg;
    int status;

    conn->send_ready = 1;
    do {
        msg = rsp_send_next(ctx, conn);
        // send
        if (msg == NULL)
            return GA_OK;

        status = msg_send_chain(ctx, conn, msg);
        if (status != GA_OK)
            return status;

    } while (conn->send_ready);

    return GA_OK;
}
