#ifndef _GAIA_CORE_H_
#define _GAIA_CORE_H_

struct array;
struct context;
struct conn;
struct server;
struct msg;
struct mbuf;
struct mhdr;
struct msg_tqh;
struct conn_tqh;

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#include <netinet/in.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include "libforestdb/forestdb.h"

#include "ga_array.h"
#include "ga_queue.h"
#include "ga_util.h"
#include "ga_log.h"
#include "ga_string.h"
#include "ga_forestdb.h"
#include "ga_signal.h"

#include "ga_client.h"
#include "ga_mbuf.h"
#include "ga_message.h"
#include "ga_connection.h"
#include "ga_event.h"
#include "ga_memcache.h"
#include "ga_net.h"
#include "ga_server.h"
#include "gaia.h"

#define GA_OK 0
#define GA_ERR -1
#define GA_EAGAIN -2
#define GA_ENOMEM -3

/*
 * Length of 1 byte, 2 bytes, 4 bytes, 8 bytes and largest integral
 * type (uintmax_t) in ascii, including the null terminator '\0'
 *
 * From stdint.h, we have:
 * # define UINT8_MAX	(255)
 * # define UINT16_MAX	(65535)
 * # define UINT32_MAX	(4294967295U)
 * # define UINT64_MAX	(__UINT64_C(18446744073709551615))
 */
#define GA_UINT8_MAXLEN (3 + 1)
#define GA_UINT16_MAXLEN (5 + 1)
#define GA_UINT32_MAXLEN (10 + 1)
#define GA_UINT64_MAXLEN (20 + 1)
#define GA_UINTMAX_MAXLEN GA_UINT64_MAXLEN

#define ga_malloc malloc
#define ga_free free
#define ga_calloc calloc

#define ga_realloc(_p, _s) realloc(_p, (size_t)(_s))
#define ga_read(_d, _b, _n) read(_d, _b, (int)(_n))
#define ga_writev(_d, _b, _n) writev(_d, _b, (int)(_n))

void core_core(struct context *ctx, struct conn *c, uint32_t events);
int core_loop(struct context *ctx);
#endif
