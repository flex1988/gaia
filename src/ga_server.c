#include "ga_core.h"

#define WORKER_THREAD 4

struct server *server_init(char *name, int port) {
    struct server *s = ga_malloc(sizeof(struct server) + sizeof(struct context) * WORKER_THREAD);
    int i;
    int status;

    s->log_level = NOTICE;
    s->sd = -1;
    s->epoll_timeout = 1;
    s->thread = WORKER_THREAD;

    for (i = 0; i < WORKER_THREAD; i++) {
        s->ctx[i].base = NULL;
        s->ctx[i].sd = 0;
        s->ctx[i].server = s;
        s->ctx[i].nfree_msgq = 0;
        s->ctx[i].frag_id = 0;
        s->ctx[i].fhandle = NULL;
        s->ctx[i].kvhandle = NULL;
        s->ctx[i].nfree_mbufq = 0;

        status = forestdb_init(&s->ctx[i]);
        ASSERT(status == GA_OK);

        TAILQ_INIT(&s->ctx[i].free_msgq);
        STAILQ_INIT(&s->ctx[i].free_mbufq);
    }

    sprintf(s->addrstr, "%s:%d", name, port);

    if (ga_resolve(name, port, &s->info) != 0) {
        ga_free(s);
        return NULL;
    }

    conn_init();
    mbuf_init();
    ga_log_init(s->log_level, "./gaia.log");

    return s;
}

int server_listen(struct server *server, struct context *ctx) {
    struct conn *s_conn;
    int status;

    ctx->sd = socket(server->info.family, SOCK_STREAM, 0);
    if (ctx->sd < 0) {
        ga_log(NOTICE, "socket create failed: %s", strerror(errno));
        return GA_ERR;
    }

    int opt = 1;
    setsockopt(ctx->sd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    if (bind(ctx->sd, (struct sockaddr *)&server->info.addr, server->info.addrlen) < 0) {
        ga_log(NOTICE, "server bind on %d to addr %s failed: %s", ctx->sd, server->addrstr, strerror(errno));
        return GA_ERR;
    }

    if (listen(ctx->sd, 512) < 0) {
        ga_log(NOTICE, "server listen failed on %d addr %s", ctx->sd, server->addrstr);
        return GA_ERR;
    }

    if (ga_set_nonblocking(ctx->sd) < 0) {
        ga_log(NOTICE, "server set nonblocking failed");
        return GA_ERR;
    }

    s_conn = conn_get(ctx->sd, false);
    s_conn->owner = ctx;

    status = event_add_conn(ctx->base->ep, s_conn);
    if (status < 0) {
        ga_log(ERROR, "event add conn e %d s %d failed: %s", ctx->base->ep, server->sd, strerror(errno));
        return GA_ERR;
    }

    status = event_del_out(ctx->base->ep, s_conn);
    if (status < 0) {
        ga_log(ERROR, "event del conn e %d s %d failed: %s", ctx->base->ep, ctx->sd, strerror(errno));
        return GA_ERR;
    }

    return GA_OK;
}

static int server_accept(struct context *ctx, struct conn *s_conn) {
    int status;
    struct conn *c_conn;
    int sd;

    for (;;) {
        sd = accept(s_conn->sd, NULL, NULL);

        if (sd < 0) {
            if (errno == EINTR) {
                ga_log(NOTICE, "accept on s %d not ready - eintr");
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                ga_log(NOTICE, "accept on s %d not ready - eagain");
                s_conn->recv_ready = 0;
                return GA_EAGAIN;
            }

            ga_log(ERROR, "accept on s %d failed: %s", s_conn->sd, strerror(errno));
            return GA_ERR;
        }
        break;
    }
    c_conn = conn_get(sd, true);
    c_conn->owner = ctx;

    if (c_conn == NULL) {
        close(sd);
        ga_log(ERROR, "get conn for c %d from s %d failed: %s", sd, s_conn->sd, strerror(errno));
        return GA_ERR;
    }

    status = ga_set_nonblocking(sd);
    if (status < 0) {
        ga_log(ERROR, "set nonblock on c %d failed: %s", sd, strerror(errno));
        return GA_ERR;
    }

    status = ga_set_tcpnodelay(c_conn->sd);
    if (status < 0) {
        ga_log(ERROR, "set nodelay on c %d failed: %s", sd, strerror(errno));
    }

    status = ga_set_keepalive(c_conn->sd);
    if (status < 0) {
        ga_log(ERROR, "set tcp keepalive on c %d failed, ignored: %s", sd, strerror(errno));
    }

    status = event_add_conn(ctx->base->ep, c_conn);
    if (status < 0) {
        ga_log(ERROR, "event add conn e %d c %d failed, ignored: %s", ctx->base->ep, sd, strerror(errno));
    }

    status = event_del_out(ctx->base->ep, c_conn);
    if (status < 0) {
        ga_log(ERROR, "event del conn e %d s %d failed: %s", ctx->base->ep, sd, strerror(errno));
    }
    ga_log(NOTICE, "accepted c %d on s %d", c_conn->sd, s_conn->sd);

    return GA_OK;
}

int server_recv(struct context *ctx, struct conn *conn) {
    int status;

    conn->recv_ready = 1;

    do {
        status = server_accept(ctx, conn);
        if (status == GA_EAGAIN)
            break;
        if (status != GA_OK)
            return status;
        ga_log(NOTICE, "accept: %ld", ctx->thread);
    } while (conn->recv_ready);

    return GA_OK;
}

void ga_server_free(struct server *server) { ga_free(server); }
