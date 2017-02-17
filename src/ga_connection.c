#include "ga_core.h"

static uint32_t nalloc_conn;
static uint32_t nfree_conn;
static struct conn_tqh free_connq;
static pthread_mutex_t queue_lock;
static pthread_mutex_t sum_lock;

void conn_init(void) {
    ga_log(NOTICE, "conn size %d", sizeof(struct conn));
    nfree_conn = 0;
    nalloc_conn = 0;
    TAILQ_INIT(&free_connq);
    pthread_mutex_init(&queue_lock, NULL);
    pthread_mutex_init(&sum_lock, NULL);
}

static struct conn *conn_create(void) {
    struct conn *conn;

    if (!TAILQ_EMPTY(&free_connq)) {
        pthread_mutex_lock(&queue_lock);
        conn = TAILQ_FIRST(&free_connq);
        nfree_conn--;
        TAILQ_REMOVE(&free_connq, conn, tqe);
        pthread_mutex_unlock(&queue_lock);
    } else {
        conn = ga_malloc(sizeof(struct conn));
        if (conn == NULL)
            return NULL;

        pthread_mutex_lock(&sum_lock);
        ++nalloc_conn;
        pthread_mutex_unlock(&sum_lock);
    }

    return conn;
}

struct conn *conn_get(int sd, bool client) {
    struct conn *c = conn_create();

    if (c == NULL)
        return NULL;
    c->sd = sd;

    c->rmsg = NULL;
    c->smsg = NULL;

    TAILQ_INIT(&c->omsg_q);

    c->recv_bytes = 0;
    c->send_bytes = 0;
    c->client = client;
    c->send_ready = 0;
    c->send_active = 0;
    c->recv_ready = 0;
    c->recv_active = 0;
    c->done = 0;
    c->err = 0;
    c->eof = 0;

    if (client) {
        c->recv = msg_recv;
        c->send = msg_send;
        c->close = client_close;
    } else {
        c->recv = server_recv;
        c->send = NULL;
        c->close = NULL;
    }

    return c;
}

void conn_put(struct conn *conn) { ga_log(NOTICE, "put conn %d", conn->sd); }

int conn_recv(struct conn *conn, void *buf, size_t size) {
    ssize_t n;

    for (;;) {
        n = ga_read(conn->sd, buf, size);

        if (n > 0) {
            if (n < (ssize_t)size) {
                conn->recv_ready = 0;
            }
            conn->recv_bytes += (size_t)n;
            return n;
        }

        if (n == 0) {
            conn->recv_ready = 0;
            conn->eof = 1;
            return n;
        }

        if (errno == EINTR) {
            ga_log(ERROR, "recv on sd %d not ready - eintr", conn->sd);
            continue;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            conn->recv_ready = 0;
            ga_log(ERROR, "recv on sd %d not ready - eagain", conn->sd);
            return GA_EAGAIN;
        } else {
            conn->recv_ready = 0;
            conn->err = errno;
            ga_log(ERROR, "recv on sd %d failed: %s", conn->sd, strerror(errno));
            return GA_ERR;
        }
    }

    return GA_ERR;
}

ssize_t conn_sendv(struct conn *conn, struct array *sendv, size_t nsend) {
    ssize_t n;

    for (;;) {
        n = ga_writev(conn->sd, sendv->elem, sendv->nelem);
        if (n > 0) {
            if (n < (ssize_t) nsend) {
                conn->send_ready = 0;
            }
            conn->send_bytes += (size_t)n;
            return n;
        }

        if (n == 0) {
            ga_log(NOTICE, "sendv on sd %d return zero", conn->sd);
            conn->send_ready = 0;
            return 0;
        }

        if (errno == EINTR) {
            ga_log(NOTICE, "sendv on sd %d not ready - eintr", conn->sd);
            continue;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            conn->send_ready = 0;
            ga_log(NOTICE, "sendv on sd %d not ready - eagain", conn->sd);
            return GA_EAGAIN;
        } else {
            conn->send_ready = 0;
            conn->err = errno;
            ga_log(NOTICE, "sendv on sd %d failed: %s", conn->sd, strerror(errno));
            return GA_ERR;
        }
    }

    return GA_ERR;
}
