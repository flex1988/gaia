#include <sys/epoll.h>
#include "ga_core.h"

struct event_base *event_base_create(int nevent) {
    struct event_base *evb;
    int status, ep;
    struct epoll_event *event;

    ep = epoll_create(nevent);
    if (ep < 0) {
        ga_log(ERROR, "epoll create of size %d failed: %s", nevent, strerror(errno));
        return NULL;
    }

    event = ga_calloc(nevent, sizeof(*event));
    if (event == NULL) {
        status = close(ep);
        if (status < 0) {
            ga_log(ERROR, "close epoll %d failed %s", ep, strerror(errno));
        }
        return NULL;
    }

    evb = ga_malloc(sizeof(*evb));
    if (evb == NULL) {
        ga_free(event);
        status = close(ep);
        if (status < 0) {
            ga_log(ERROR, "close event %d failed %s", ep, strerror(errno));
        }
        return NULL;
    }

    evb->ep = ep;
    evb->event = event;
    evb->nevent = nevent;

    return evb;
}

int event_wait(struct event_base *evb, int timeout) {
    int ep = evb->ep;
    struct epoll_event *event = evb->event;
    int nevent = evb->nevent;

    for (;;) {
        int nsd;

        nsd = epoll_wait(ep, event, nevent, timeout);

        if (nsd > 0)
            return nsd;

        if (nsd == 0)
            return 0;

        if (errno == EINTR)
            continue;

        ga_log(ERROR, "epoll wait on e %d with %d events failed: %s", ep, nevent, strerror(errno));
        return -1;
    }
}

int event_add_conn(int ep, struct conn *conn) {
    int status;
    struct epoll_event event;

    event.events = (EPOLLIN | EPOLLOUT | EPOLLET);
    event.data.ptr = conn;

    status = epoll_ctl(ep, EPOLL_CTL_ADD, conn->sd, &event);
    if (status < 0) {
        ga_log(ERROR, "epoll ctl add on e %d sd %d failed: %s", ep, conn->sd, strerror(errno));
    }

    return status;
}

int event_del_conn(int ep, struct conn *conn) {
    int status;

    status = epoll_ctl(ep, EPOLL_CTL_DEL, conn->sd, NULL);

    if (status < 0) {
        ga_log(ERROR, "epoll ctl del on e %d sd %d failed: %s", ep, conn->sd, strerror(errno));
    }

    return status;
}

int event_del_out(int ep, struct conn *c) {
    int status;
    struct epoll_event event;

    if (!c->send_active) {
        return 0;
    }

    event.events = (uint32_t)(EPOLLIN | EPOLLET);
    event.data.ptr = c;

    status = epoll_ctl(ep, EPOLL_CTL_MOD, c->sd, &event);

    if (status < 0) {
        ga_log(ERROR, "epoll ctl on e %d sd %d failed: %s", ep, c->sd, strerror(errno));
    } else {
        c->send_active = 0;
    }

    return status;
}

int event_add_out(int ep, struct conn *c) {
    int status;
    struct epoll_event event;

    if (c->send_active) {
        return 0;
    }

    event.events = (uint32_t)(EPOLLIN | EPOLLOUT | EPOLLET);
    event.data.ptr = c;

    status = epoll_ctl(ep, EPOLL_CTL_MOD, c->sd, &event);
    if (status < 0) {
        ga_log(ERROR, "epoll ctl on e %d sd %d failed: %s", ep, c->sd, strerror(errno));
    } else {
        c->send_active = 1;
    }

    return status;
}
