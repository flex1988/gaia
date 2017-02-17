#ifndef _GAIA_MEMCACHE_H_
#define _GAIA_MEMCACHE_H_

#include "ga_core.h"

#define strcrlf(m) (*(m) == '\r' && *((m) + 1) == '\n')

#define str4cmp(m, c0, c1, c2, c3) (*(uint32_t *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0))

#define str5cmp(m, c0, c1, c2, c3, c4) (str4cmp(m, c0, c1, c2, c3) && (m[4] == c4))

#define str6cmp(m, c0, c1, c2, c3, c4, c5) (str4cmp(m, c0, c1, c2, c3) && (((uint32_t *)m)[1] & 0xffff) == ((c5 << 8) | c4))

#define str7cmp(m, c0, c1, c2, c3, c4, c5, c6) (str6cmp(m, c0, c1, c2, c3, c4, c5) && (m[6] == c6))

#define str8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7) \
    (str4cmp(m, c0, c1, c2, c3) && (((uint32_t *)m)[1] == ((c7 << 24) | (c6 << 16) | (c5 << 8) | c4)))

#define str9cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8) (str8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7) && m[8] == c8)

#define str10cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9) \
    (str8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7) && (((uint32_t *)m)[2] & 0xffff) == ((c9 << 8) | c8))

#define str11cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10) (str10cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9) && (m[10] == c10))

#define str12cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11) \
    (str8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7) && (((uint32_t *)m)[2] == ((c11 << 24) | (c10 << 16) | (c9 << 8) | c8)))

void memcache_parse_req(struct msg *req);
#endif
