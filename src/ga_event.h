#ifndef _GAIA_EVENT_H_
#define _GAIA_EVENT_H_

#include "ga_core.h"

#define EVENT_SIZE 1024

#define EVENT_READ 0x0000ff
#define EVENT_WRITE 0x00ff00
#define EVENT_ERR 0xff0000

typedef int (*event_cb_t)(void *, uint32_t);

struct event_base
{
    int ep; /* epoll descriptor */

    struct epoll_event *event;
    int nevent;
};

struct event_base *event_base_create(int size);
void event_base_destroy(struct event_base *evb);

int event_wait(struct event_base *evb, int timeout);
int event_add_conn(int ep, struct conn *conn);
int event_del_conn(int ep, struct conn *conn);
int event_del_out(int ep, struct conn *c);
int event_add_out(int ep,struct conn *c);
#endif
