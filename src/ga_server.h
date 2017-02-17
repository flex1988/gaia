#ifndef _GAIA_SERVER_H_
#define _GAIA_SERVER_H_

struct context {
    int sd;
    struct server *server;
    pthread_t thread;
    struct event_base *base;

    struct msg_tqh free_msgq;
    uint32_t nfree_msgq;
    uint32_t frag_id;

    struct mhdr free_mbufq;
    uint32_t nfree_mbufq;

    fdb_file_handle *fhandle;
    fdb_kvs_handle *kvhandle;
    fdb_config config;
    fdb_kvs_config kvs_config;
};

struct server {
    int log_level;
    int sd;
    int epoll_timeout;
    int thread;
    char addrstr[30];
    struct sockinfo info;
    struct context ctx[0];
};

struct server *server_init(char *name, int port);
int server_listen(struct server *server, struct context *ctx);
void ga_server_free(struct server *server);
int server_recv(struct context *ctx, struct conn *conn);
#endif
