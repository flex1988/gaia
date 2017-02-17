#include <ctype.h>
#include "ga_core.h"

#define MEMCACHE_MAX_KEY_LENGTH 250

/*
 * Return true, if the memcache command takes a key argument, otherwise
 * return false
 */
static bool memcache_key(struct msg *r) {
    switch (r->type) {
        case MSG_REQ_GET:
        case MSG_REQ_GETS:
        case MSG_REQ_DELETE:
        case MSG_REQ_CAS:
        case MSG_REQ_SET:
        case MSG_REQ_ADD:
        case MSG_REQ_REPLACE:
        case MSG_REQ_APPEND:
        case MSG_REQ_PREPEND:
        case MSG_REQ_INCR:
        case MSG_REQ_DECR:
            return true;

        default:
            break;
    }

    return false;
}

/*
 * Return true, if the memcache command is a quit command, otherwise
 * return false
 */
static bool memcache_quit(struct msg *r) {
    if (r->type == MSG_REQ_QUIT) {
        return true;
    }

    return false;
}

/*
 * Return true, if the memcache command is a version command, otherwise
 * return false
 */
static bool memcache_version(struct msg *r) {
    if (r->type == MSG_REQ_VERSION) {
        return true;
    }

    return false;
}

/*
 * Return true, if the memcache command is a storage command, otherwise
 * return false
 */
static bool memcache_storage(struct msg *r) {
    switch (r->type) {
        case MSG_REQ_SET:
        case MSG_REQ_CAS:
        case MSG_REQ_ADD:
        case MSG_REQ_REPLACE:
        case MSG_REQ_APPEND:
        case MSG_REQ_PREPEND:
            return true;

        default:
            break;
    }

    return false;
}

/*
 * Return true, if the memcache command is a arithmetic command, otherwise
 * return false
 */
static bool memcache_arithmetic(struct msg *r) {
    switch (r->type) {
        case MSG_REQ_INCR:
        case MSG_REQ_DECR:
            return true;

        default:
            break;
    }

    return false;
}

/*
 * Return true, if the memcache command is a delete command, otherwise
 * return false
 */
static bool memcache_delete(struct msg *r) {
    if (r->type == MSG_REQ_DELETE) {
        return true;
    }

    return false;
}

static bool memcache_stats(struct msg *r) { return r->type == MSG_REQ_STATS; }

/*
 * Return true, if the memcache command is a retrieval command, otherwise
 * return false
 */
static bool memcache_retrieval(struct msg *r) {
    switch (r->type) {
        case MSG_REQ_GET:
        case MSG_REQ_GETS:
            return true;

        default:
            break;
    }

    return false;
}

/*
 * Return true, if the memcache command is a cas command, otherwise
 * return false
 */
static bool memcache_cas(struct msg *r) {
    if (r->type == MSG_REQ_CAS) {
        return true;
    }

    return false;
}

void memcache_parse_req(struct msg *r) {
    struct mbuf *b;
    uint8_t *p, *m;
    uint8_t ch;
    enum {
        SW_START,
        SW_REQ_TYPE,
        SW_SPACES_BEFORE_KEY,
        SW_KEY,
        SW_SPACES_BEFORE_KEYS,
        SW_SPACES_BEFORE_FLAGS,
        SW_FLAGS,
        SW_SPACES_BEFORE_EXPIRY,
        SW_EXPIRY,
        SW_SPACES_BEFORE_VLEN,
        SW_VLEN,
        SW_SPACES_BEFORE_CAS,
        SW_CAS,
        SW_RUNTO_VAL,
        SW_VAL,
        SW_SPACES_BEFORE_NUM,
        SW_NUM,
        SW_RUNTO_CRLF,
        SW_CRLF,
        SW_NOREPLY,
        SW_AFTER_NOREPLY,
        SW_ALMOST_DONE,
        SW_SENTINEL
    } state;

    state = r->state;
    b = STAILQ_LAST(&r->mhdr, mbuf, next);
    r->result = MSG_PARSE_OK;
    r->type = MSG_REQ_GET;

    for (p = r->pos; p < b->last; p++) {
        ch = *p;

        switch (state) {
            case SW_START:
                if (ch == ' ') {
                    break;
                }

                if (!islower(ch)) {
                    goto error;
                }

                /* req_start <- p; type_start <- p */
                r->token = p;
                state = SW_REQ_TYPE;

                break;
            case SW_REQ_TYPE:
                if (ch == ' ' || ch == CR) {
                    /* type_end = p - 1 */
                    m = r->token;
                    r->token = NULL;
                    r->type = MSG_UNKNOWN;

                    switch (p - m) {
                        case 3:
                            if (str4cmp(m, 'g', 'e', 't', ' ')) {
                                r->type = MSG_REQ_GET;
                                break;
                            }

                            if (str4cmp(m, 's', 'e', 't', ' ')) {
                                r->type = MSG_REQ_SET;
                                break;
                            }

                            if (str4cmp(m, 'a', 'd', 'd', ' ')) {
                                r->type = MSG_REQ_ADD;
                                break;
                            }

                            if (str4cmp(m, 'c', 'a', 's', ' ')) {
                                r->type = MSG_REQ_CAS;
                                break;
                            }

                            break;

                        case 4:
                            if (str4cmp(m, 'g', 'e', 't', 's')) {
                                r->type = MSG_REQ_GETS;
                                break;
                            }

                            if (str4cmp(m, 'i', 'n', 'c', 'r')) {
                                r->type = MSG_REQ_INCR;
                                break;
                            }

                            if (str4cmp(m, 'd', 'e', 'c', 'r')) {
                                r->type = MSG_REQ_DECR;
                                break;
                            }

                            if (str4cmp(m, 'q', 'u', 'i', 't')) {
                                r->type = MSG_REQ_QUIT;
                                r->quit = 1;
                                break;
                            }

                            break;

                        case 5:
                            if (str5cmp(m, 's', 't', 'a', 't', 's')) {
                                r->type = MSG_REQ_STATS;
                                break;
                            }
                            break;
                        case 6:
                            if (str6cmp(m, 'a', 'p', 'p', 'e', 'n', 'd')) {
                                r->type = MSG_REQ_APPEND;
                                break;
                            }

                            if (str6cmp(m, 'd', 'e', 'l', 'e', 't', 'e')) {
                                r->type = MSG_REQ_DELETE;
                                break;
                            }

                            break;

                        case 7:
                            if (str7cmp(m, 'p', 'r', 'e', 'p', 'e', 'n', 'd')) {
                                r->type = MSG_REQ_PREPEND;
                                break;
                            }

                            if (str7cmp(m, 'r', 'e', 'p', 'l', 'a', 'c', 'e')) {
                                r->type = MSG_REQ_REPLACE;
                                break;
                            }

                            if (str7cmp(m, 'v', 'e', 'r', 's', 'i', 'o', 'n')) {
                                r->type = MSG_REQ_VERSION;
                                break;
                            }

                            break;
                    }

                    if (memcache_key(r)) {
                        if (ch == CR) {
                            goto error;
                        }
                        state = SW_SPACES_BEFORE_KEY;
                    } else if (memcache_quit(r) || memcache_version(r)) {
                        p = p - 1; /* go back by 1 byte */
                        state = SW_CRLF;
                    } else if (memcache_stats(r)) {
                        while (*p == ' ') p++; /* trim the space */
                        if (*p == '\r') {
                            state = SW_CRLF;
                        } else {
                            state = SW_SPACES_BEFORE_KEY;
                        }
                        p = p - 1;
                    } else {
                        goto error;
                    }
                } else if (!islower(ch)) {
                    goto error;
                }

                break;

            case SW_SPACES_BEFORE_KEY:
                if (ch != ' ') {
                    p = p - 1; /* go back by 1 byte */
                    state = SW_KEY;
                }

                break;

            case SW_KEY:
                if (r->token == NULL) {
                    r->token = p;
                    r->key_start = p;
                }

                if (ch == ' ' || ch == CR) {
                    if ((p - r->key_start) > MEMCACHE_MAX_KEY_LENGTH) {
                        ga_log(DEBUG, "parsed bad req %" PRIu64
                                      " of type %d with key "
                                      "prefix '%.*s...' and length %d that exceeds "
                                      "maximum key length",
                               r->id, r->type, 16, r->key_start, p - r->key_start);
                        goto error;
                    }
                    r->key_end = p;
                    r->token = NULL;

                    /* get next state */
                    if (memcache_storage(r)) {
                        state = SW_SPACES_BEFORE_FLAGS;
                    } else if (memcache_arithmetic(r)) {
                        state = SW_SPACES_BEFORE_NUM;
                    } else if (memcache_delete(r)) {
                        state = SW_RUNTO_CRLF;
                    } else if (memcache_retrieval(r)) {
                        state = SW_SPACES_BEFORE_KEYS;
                    } else {
                        state = SW_RUNTO_CRLF;
                    }

                    if (ch == CR) {
                        if (memcache_storage(r) || memcache_arithmetic(r)) {
                            goto error;
                        }
                        p = p - 1; /* go back by 1 byte */
                    }
                }

                break;

            case SW_SPACES_BEFORE_KEYS:
                ASSERT(memcache_retrieval(r));
                switch (ch) {
                    case ' ':
                        break;

                    case CR:
                        state = SW_ALMOST_DONE;
                        break;

                    default:
                        r->token = p;
                        goto fragment;
                }

                break;

            case SW_SPACES_BEFORE_FLAGS:
                if (ch != ' ') {
                    if (!isdigit(ch)) {
                        goto error;
                    }
                    p = p - 1; /* go back by 1 byte */
                    state = SW_FLAGS;
                }

                break;

            case SW_FLAGS:
                if (r->token == NULL) {
                    /* flags_start <- p */
                    r->token = p;
                    r->flags = 0;
                }

                if (isdigit(ch)) {
                    r->flags = r->flags * 10 + (uint32_t)(ch - '0');
                } else if (ch == ' ') {
                    /* flags_end <- p - 1 */
                    r->token = NULL;
                    state = SW_SPACES_BEFORE_EXPIRY;
                } else {
                    goto error;
                }

                break;

            case SW_SPACES_BEFORE_EXPIRY:
                if (ch != ' ') {
                    if (!isdigit(ch)) {
                        goto error;
                    }
                    p = p - 1; /* go back by 1 byte */
                    state = SW_EXPIRY;
                }

                break;

            case SW_EXPIRY:
                if (r->token == NULL) {
                    /* expiry_start <- p */
                    r->token = p;
                    r->expiry = 0;
                }

                if (isdigit(ch)) {
                    r->expiry = r->expiry * 10 + (uint32_t)(ch - '0');
                } else if (ch == ' ') {
                    /* expiry_end <- p - 1 */
                    r->token = NULL;
                    state = SW_SPACES_BEFORE_VLEN;
                } else {
                    goto error;
                }

                break;

            case SW_SPACES_BEFORE_VLEN:
                if (ch != ' ') {
                    if (!isdigit(ch)) {
                        goto error;
                    }
                    p = p - 1; /* go back by 1 byte */
                    state = SW_VLEN;
                }

                break;

            case SW_VLEN:
                if (r->token == NULL) {
                    /* vlen_start <- p */
                    r->token = p;
                    r->vlen = 0;
                }

                if (isdigit(ch)) {
                    r->vlen = r->vlen * 10 + (uint32_t)(ch - '0');
                } else if (memcache_cas(r)) {
                    if (ch != ' ') {
                        goto error;
                    }
                    /* vlen_end <- p - 1 */
                    r->rvlen = r->vlen;
                    p = p - 1; /* go back by 1 byte */
                    r->token = NULL;
                    state = SW_SPACES_BEFORE_CAS;
                } else if (ch == ' ' || ch == CR) {
                    /* vlen_end <- p - 1 */
                    r->rvlen = r->vlen;
                    p = p - 1; /* go back by 1 byte */
                    r->token = NULL;
                    state = SW_RUNTO_CRLF;
                } else {
                    goto error;
                }

                break;

            case SW_SPACES_BEFORE_CAS:
                if (ch != ' ') {
                    if (!isdigit(ch)) {
                        goto error;
                    }
                    p = p - 1; /* go back by 1 byte */
                    state = SW_CAS;
                }

                break;

            case SW_CAS:
                if (r->token == NULL) {
                    /* cas_start <- p */
                    r->token = p;
                    r->cas = 0;
                }

                if (isdigit(ch)) {
                    r->cas = r->cas * 10ULL + (uint64_t)(ch - '0');
                } else if (ch == ' ' || ch == CR) {
                    /* cas_end <- p - 1 */
                    p = p - 1; /* go back by 1 byte */
                    r->token = NULL;
                    state = SW_RUNTO_CRLF;
                } else {
                    goto error;
                }

                break;

            case SW_RUNTO_VAL:
                switch (ch) {
                    case LF:
                        /* val_start <- p + 1 */
                        state = SW_VAL;
                        break;

                    default:
                        goto error;
                }

                break;

            case SW_VAL:
                if (r->value == NULL) {
                    r->value = p;
                }
                m = p + r->rvlen;
                if (m >= b->last) {
                    ASSERT(r->rvlen >= (uint32_t)(b->last - p));
                    r->rvlen -= (uint32_t)(b->last - p);
                    m = b->last - 1;
                    p = m; /* move forward by vlen bytes */
                    break;
                }
                switch (*m) {
                    case CR:
                        /* val_end <- p - 1 */
                        p = m; /* move forward by vlen bytes */
                        state = SW_ALMOST_DONE;
                        break;

                    default:
                        goto error;
                }

                break;

            case SW_SPACES_BEFORE_NUM:
                if (ch != ' ') {
                    if (!isdigit(ch)) {
                        goto error;
                    }
                    p = p - 1; /* go back by 1 byte */
                    state = SW_NUM;
                }

                break;

            case SW_NUM:
                if (r->token == NULL) {
                    /* num_start <- p */
                    r->token = p;
                    r->num = 0;
                }

                if (isdigit(ch)) {
                    r->num = r->num * 10ULL + (uint64_t)(ch - '0');
                } else if (ch == ' ' || ch == CR) {
                    r->token = NULL;
                    /* num_end <- p - 1 */
                    p = p - 1; /* go back by 1 byte */
                    state = SW_RUNTO_CRLF;
                } else {
                    goto error;
                }

                break;

            case SW_RUNTO_CRLF:
                switch (ch) {
                    case ' ':
                        break;

                    case 'n':
                        if (memcache_storage(r) || memcache_arithmetic(r) || memcache_delete(r)) {
                            p = p - 1; /* go back by 1 byte */
                            state = SW_NOREPLY;
                        } else {
                            goto error;
                        }

                        break;

                    case CR:
                        if (memcache_storage(r)) {
                            state = SW_RUNTO_VAL;
                        } else {
                            state = SW_ALMOST_DONE;
                        }

                        break;

                    default:
                        goto error;
                }

                break;

            case SW_NOREPLY:
                if (r->token == NULL) {
                    /* noreply_start <- p */
                    r->token = p;
                }

                switch (ch) {
                    case ' ':
                    case CR:
                        m = r->token;
                        if (((p - m) == 7) && str7cmp(m, 'n', 'o', 'r', 'e', 'p', 'l', 'y')) {
                            ASSERT(memcache_storage(r) || memcache_arithmetic(r) || memcache_delete(r));
                            r->token = NULL;
                            /* noreply_end <- p - 1 */
                            r->noreply = 1;
                            state = SW_AFTER_NOREPLY;
                            p = p - 1; /* go back by 1 byte */
                        } else {
                            goto error;
                        }
                }

                break;

            case SW_AFTER_NOREPLY:
                switch (ch) {
                    case ' ':
                        break;

                    case CR:
                        if (memcache_storage(r)) {
                            state = SW_RUNTO_VAL;
                        } else {
                            state = SW_ALMOST_DONE;
                        }
                        break;

                    default:
                        goto error;
                }

                break;

            case SW_CRLF:
                switch (ch) {
                    case ' ':
                        break;

                    case CR:
                        state = SW_ALMOST_DONE;
                        break;

                    default:
                        goto error;
                }

                break;

            case SW_ALMOST_DONE:
                switch (ch) {
                    case LF:
                        /* req_end <- p */
                        goto done;

                    default:
                        goto error;
                }

                break;

            case SW_SENTINEL:
            default:
                ASSERT(0);
                break;
        }
    }
    ASSERT(p == b->last);
    r->pos = p;
    r->state = state;

    if (b->last == b->end && r->token != NULL) {
        r->pos = r->token;
        r->token = NULL;
        r->result = MSG_PARSE_REPAIR;
    } else {
        r->result = MSG_PARSE_AGAIN;
    }

    ga_log(NOTICE, "parsed req %" PRIu64 " res %d type %d state %d rpos %d of %d", r->id, r->result, r->type, r->state, r->pos - b->pos,
           b->last - b->pos);
    return;

fragment:
    ASSERT(p != b->last);
    ASSERT(r->token != NULL);
    r->pos = r->token;
    r->token = NULL;
    r->state = state;
    r->result = MSG_PARSE_FRAGMENT;

    return;

done:
    ASSERT(r->type > MSG_UNKNOWN && r->type < MSG_SENTINEL);
    r->pos = p + 1;
    ASSERT(r->pos <= b->last);
    r->state = SW_START;
    r->result = MSG_PARSE_OK;

    ga_log(DEBUG, "parsed req %" PRIu64
                  " res %d "
                  "type %d state %d rpos %d of %d",
           r->id, r->result, r->type, r->state, r->pos - b->pos, b->last - b->pos);
    return;

error:
    r->result = MSG_PARSE_ERROR;
    r->state = state;
    errno = EINVAL;

    ga_log(DEBUG, "parsed bad req %" PRIu64 " res %d type %d state %d", r->id, r->result, r->type, r->state);
}
