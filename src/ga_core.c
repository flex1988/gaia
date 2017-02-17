#include "ga_core.h"
#include <sys/epoll.h>

static void core_close(struct context *ctx, struct conn *conn) {
    int status;

    status = event_del_conn(ctx->base->ep, conn);
    if (status < 0) {
        ga_log(WARN, "event del conn e %d %d failed: %s", ctx->base->ep, conn->sd, strerror(errno));
    }

    conn->close(ctx, conn);
}

void core_core(struct context *ctx, struct conn *conn, uint32_t events) {
    int status;
    conn->events = events;

    // error
    if (events & EPOLLERR) {
        // handle error
        ga_log(ERROR, "epoll error");
        return;
    }

    // read
    if (events & (EPOLLIN | EPOLLHUP)) {
        // handle read
        status = conn->recv(ctx, conn);

        if (status != GA_OK || conn->done || conn->err) {
            ga_log(DEBUG, "recv socket %d failed: %s", conn->sd, strerror(errno));
            core_close(ctx, conn);
            return;
        }
    }

    if (events & EPOLLOUT) {
        // handle write
        status = conn->send(ctx, conn);

        if (status != GA_OK || conn->done || conn->err) {
            // close conn
            ga_log(DEBUG, "recv socket %d failed: %s", conn->sd, strerror(errno));
            core_close(ctx, conn);
            return;
        }
    }
}

int core_loop(struct context *ctx) {
    int i, nsd;

    nsd = event_wait(ctx->base, ctx->server->epoll_timeout);
    if (nsd < 0) {
        return nsd;
    }

    for (i = 0; i < nsd; i++) {
        struct epoll_event *ev = &ctx->base->event[i];
        core_core(ctx, ev->data.ptr, ev->events);
    }

    return GA_OK;
}
