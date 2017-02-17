#ifndef _GA_UTIL_H_
#define _GA_UTIL_H_

#define LF (uint8_t)10
#define CR (uint8_t)13
#define CRLF "\r\n"
#define CRLF_LEN (uint32_t)(sizeof(CRLF) - 1)

#define GA_NOT_USED(_v) (void)(_v)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ASSERT(_x)                                 \
    do {                                           \
        if (!(_x)) {                               \
            ga_assert(#_x, __FILE__, __LINE__, 1); \
        }                                          \
    } while (0)

void ga_assert(const char *cond, const char *file, int line, int panic);
void ga_stacktrace(int skip_count);
#endif
