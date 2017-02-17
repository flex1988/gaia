#include "ga_core.h"

void client_close(struct context *ctx, struct conn *conn) {
    int status;

    if (conn->sd < 0) {
        conn_put(conn);
        return;
    }

    status = close(conn->sd);
    if (status < 0) {
        ga_log(ERROR, "close c %d failed, ignored: %s", conn->sd, strerror(errno));
    }
    conn->sd = -1;
    conn_put(conn);
}
