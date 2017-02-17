#include "gaia.h"

typedef void *(*thread_routine_t)(void *);

void exit_server(struct server *server, char *err) {
    ga_server_free(server);
    ga_log(ERROR, "gaia server exit: %s", err);
    exit(1);
}

void *thread_routine(void *arg) {
    struct context *ctx = (struct context *)arg;
    int status;

    ctx->thread = pthread_self();
    ctx->base = event_base_create(EVENT_SIZE);

    status = server_listen(ctx->server, ctx);
    
    ga_log(NOTICE,"thread start: %ld",ctx->thread);
    /* core loop */
    for (;;) {
        if (core_loop(ctx) != GA_OK) {
            break;
        }
    }

    return NULL;
}

int spawn(pthread_t *thread, struct context *ctx, thread_routine_t func) {
    int status;
    status = pthread_create(thread, NULL, func, (void *)ctx);
    if (status != GA_OK) {
        ga_log(ERROR, "pthread_create failed: %s", strerror(errno));
        return status;
    }
    return GA_OK;
}

int main(int argc, char **argv) {
    int i, status;
    struct server *server = server_init("localhost", 8080);

    if (!server) {
        ga_log(ERROR, "gaia server init failed");
        exit(1);
    }

    for (i = 0; i < server->thread; i++) {
        status = spawn(&server->ctx[i].thread, &server->ctx[i], thread_routine);
        if (status != GA_OK) {
            ga_log(ERROR, "pthread_create failed: %s", strerror(errno));
            return GA_ERR;
        }
    }

    for (i = 0; i < server->thread; i++) {
        status = pthread_join(server->ctx[i].thread, NULL);
        if (status != GA_OK) {
            ga_log(ERROR, "pthread_join failed: %s", strerror(errno));
            return GA_ERR;
        }
    }

    ga_log(NOTICE, "gaia server now listening on %s", server->addrstr);

    return 0;
}
