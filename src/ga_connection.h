#ifndef _GAIA_CONNECTION_H_
#define _GAIA_CONNECTION_H_

typedef int (*recv_t)(struct context *, struct conn *);
typedef int (*send_t)(struct context *, struct conn *);
typedef void (*close_t)(struct context *, struct conn *);

struct conn {
    int sd;
    TAILQ_ENTRY(conn) tqe;
    struct msg_tqh omsg_q;

    uint32_t events;
    size_t recv_bytes;
    size_t send_bytes;

    struct msg *smsg;
    struct msg *rmsg;

    struct context *owner;

    recv_t recv;
    send_t send;
    close_t close;

    unsigned recv_ready : 1;
    unsigned recv_active : 1;
    unsigned send_ready : 1;
    unsigned send_active : 1;
    unsigned client : 1;
    unsigned done : 1;
    unsigned err : 1;
    unsigned eof : 1;
};

TAILQ_HEAD(conn_tqh, conn);

struct conn *conn_get(int sd, bool client);
int conn_recv(struct conn *conn, void *buf, size_t size);
void conn_put(struct conn *conn);
ssize_t conn_sendv(struct conn *conn, struct array *sendv, size_t nsend);
void conn_init(void);
#endif
