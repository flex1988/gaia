#ifndef _GAIA_STRING_H_
#define _GAIA_STRING_H_

#include <string.h>
#include "ga_core.h"

struct string {
    uint32_t len;   /* string length */
    uint8_t  *data; /* string data */
};

#define string(_str)   { sizeof(_str) - 1, (uint8_t *)(_str) }
#define null_string    { 0, NULL }

#define string_set_text(_str, _text) do {       \
    (_str)->len = (uint32_t)(sizeof(_text) - 1);\
    (_str)->data = (uint8_t *)(_text);          \
} while (0);

#define string_set_raw(_str, _raw) do {         \
    (_str)->len = (uint32_t)(ga_strlen(_raw));  \
    (_str)->data = (uint8_t *)(_raw);           \
} while (0);

void string_init(struct string *str);
void string_deinit(struct string *str);
bool string_empty(const struct string *str);
int string_duplicate(struct string *dst, const struct string *src);
int string_copy(struct string *dst, const uint8_t *src, uint32_t srclen);
int string_compare(const struct string *s1, const struct string *s2);

/*
 * Wrapper around common routines for manipulating C character
 * strings
 */
#define ga_memcpy(_d, _c, _n)           \
    memcpy(_d, _c, (size_t)(_n))

#define ga_memmove(_d, _c, _n)          \
    memmove(_d, _c, (size_t)(_n))

#define ga_memchr(_d, _c, _n)           \
    memchr(_d, _c, (size_t)(_n))

#define ga_strlen(_s)                   \
    strlen((char *)(_s))

#define ga_strncmp(_s1, _s2, _n)        \
    strncmp((char *)(_s1), (char *)(_s2), (size_t)(_n))

#define ga_strchr(_p, _l, _c)           \
    _ga_strchr((uint8_t *)(_p), (uint8_t *)(_l), (uint8_t)(_c))

#define ga_strrchr(_p, _s, _c)          \
    _ga_strrchr((uint8_t *)(_p),(uint8_t *)(_s), (uint8_t)(_c))

#define ga_strndup(_s, _n)              \
    (uint8_t *)strndup((char *)(_s), (size_t)(_n));

#define ga_snprintf(_s, _n, ...)        \
    snprintf((char *)(_s), (size_t)(_n), __VA_ARGS__)

#define ga_scnprintf(_s, _n, ...)       \
    _scnprintf((char *)(_s), (size_t)(_n), __VA_ARGS__)

#define ga_vscnprintf(_s, _n, _f, _a)   \
    _vscnprintf((char *)(_s), (size_t)(_n), _f, _a)

static inline uint8_t *
_ga_strchr(uint8_t *p, uint8_t *last, uint8_t c)
{
    while (p < last) {
        if (*p == c) {
            return p;
        }
        p++;
    }

    return NULL;
}

static inline uint8_t *
_ga_strrchr(uint8_t *p, uint8_t *start, uint8_t c)
{
    while (p >= start) {
        if (*p == c) {
            return p;
        }
        p--;
    }

    return NULL;
}

#endif
