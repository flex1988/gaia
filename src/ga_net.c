#include <fcntl.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "ga_core.h"

static int ga_resolve_inet(char *name, int port, struct sockinfo *si) {
    int status;
    struct addrinfo *ai, *cai;
    struct addrinfo hints;
    char service[GA_UINTMAX_MAXLEN];
    bool found;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;

    snprintf(service, GA_UINTMAX_MAXLEN, "%d", port);

    status = getaddrinfo(name, service, &hints, &ai);
    if (status != 0) {
        ga_log_stderr("address resolve of node %s service %s failed: %s", name, service, gai_strerror(status));
        return GA_ERR;
    }

    for (cai = ai, found = false; cai != NULL; cai = cai->ai_next) {
        si->family = cai->ai_family;
        si->addrlen = cai->ai_addrlen;
        memcpy(&si->addr, cai->ai_addr, si->addrlen);
        found = true;
        break;
    }

    freeaddrinfo(ai);

    return !found ? -1 : 0;
}

int ga_resolve(char *name, int port, struct sockinfo *si) { return ga_resolve_inet(name, port, si); }

int ga_set_nonblocking(int sd) {
    int flags;

    flags = fcntl(sd, F_GETFL, 0);
    if (flags < 0) {
        return flags;
    }

    return fcntl(sd, F_SETFL, flags | O_NONBLOCK);
}

int ga_set_tcpnodelay(int sd) {
    int nodelay;
    socklen_t len;

    nodelay = 1;
    len = sizeof(nodelay);

    return setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &nodelay, len);
}

int ga_set_keepalive(int sd) {
    int keepalive;
    socklen_t len;

    keepalive = 1;
    len = sizeof(keepalive);

    return setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, len);
}
