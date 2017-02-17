#ifndef _GAIA_NET_H_
#define _GAIA_NET_H_

#include <pthread.h>

struct sockinfo
{
    int family;
    socklen_t addrlen;
    union
    {
        struct sockaddr_in in;
        struct sockaddr_in6 in6;
        struct sockaddr_un un;
    } addr;
};

int ga_resolve(char *name, int port, struct sockinfo *si);
int ga_set_nonblocking(int sd);
int ga_set_tcpnodelay(int sd);
int ga_set_keepalive(int sd);
#endif
