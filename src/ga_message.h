#ifndef _GAIA_MESSAGE_H_
#define _GAIA_MESSAGE_H_

typedef void (*msg_parse_t)(struct msg *);

typedef enum msg_parse_result {
    MSG_PARSE_OK,       /* parsing ok */
    MSG_PARSE_ERROR,    /* parsing error */
    MSG_PARSE_REPAIR,   /* more to parse -> repair parsed & unparsed data */
    MSG_PARSE_FRAGMENT, /* multi-vector request -> fragment */
    MSG_PARSE_AGAIN,    /* incomplete -> parse again */
} msg_parse_result_t;

#define MSG_CODEC(ACTION)                       \
    ACTION(UNKNOWN, "" /* unknown */)           \
    ACTION(REQ_GET, "get  ")                    \
    ACTION(REQ_GETS, "gets ")                   \
    ACTION(REQ_DELETE, "delete ")               \
    ACTION(REQ_CAS, "cas ")                     \
    ACTION(REQ_SET, "set ")                     \
    ACTION(REQ_ADD, "add ")                     \
    ACTION(REQ_REPLACE, "replace ")             \
    ACTION(REQ_APPEND, "append  ")              \
    ACTION(REQ_PREPEND, "prepend ")             \
    ACTION(REQ_INCR, "incr ")                   \
    ACTION(REQ_DECR, "decr ")                   \
    ACTION(REQ_STATS, "stats ")                 \
    ACTION(REQ_VERSION, "version ")             \
    ACTION(REQ_QUIT, "quit ")                   \
    ACTION(RSP_NUM, "" /* na */)                \
    ACTION(RSP_VALUE, "VALUE ")                 \
    ACTION(RSP_END, "END\r\n")                  \
    ACTION(RSP_STORED, "STORED\r\n")            \
    ACTION(RSP_NOT_STORED, "NOT_STORED\r\n")    \
    ACTION(RSP_EXISTS, "EXISTS\r\n")            \
    ACTION(RSP_NOT_FOUND, "NOT_FOUND\r\n")      \
    ACTION(RSP_DELETED, "DELETED\r\n")          \
    ACTION(RSP_CLIENT_ERROR, "CLIENT_ERROR ")   \
    ACTION(RSP_SERVER_ERROR, "SERVER_ERROR ")   \
    ACTION(RSP_VERSION, "VERSION fatcache\r\n") \
    ACTION(CRLF, "\r\n" /* empty */)            \
    ACTION(EMPTY, "" /* empty */)

#define DEFINE_ACTION(_hash, _name) MSG_##_hash,
typedef enum msg_type { MSG_CODEC(DEFINE_ACTION) MSG_SENTINEL } msg_type_t;
#undef DEFINE_ACTION

struct msg {
    uint64_t id;
    TAILQ_ENTRY(msg) m_tqe;
    TAILQ_ENTRY(msg) c_tqe;

    int state;
    uint8_t *pos;
    uint8_t *token;
    uint8_t *key_start;
    uint8_t *key_end;

    msg_parse_t parser;
    msg_parse_result_t result;
    msg_type_t type;

    struct conn *owner;

    struct msg *peer;
    struct mhdr mhdr;

    uint32_t mlen;

    uint32_t flags;
    uint32_t expiry;
    uint32_t vlen;
    uint32_t rvlen;

    uint64_t num;

    uint8_t *value;
    uint64_t cas;

    uint32_t nfrag;
    uint64_t frag_id;

    int err;
    unsigned error : 1;
    unsigned done : 1;
    unsigned noreply : 1;
    unsigned quit : 1;
    unsigned request : 1;
    unsigned last_fragment : 1;
};

TAILQ_HEAD(msg_tqh, msg);

int msg_recv(struct context *context, struct conn *conn);
int msg_send(struct context *context, struct conn *conn);

struct msg *req_recv_next(struct context *context, struct conn *conn, bool alloc);
void rsp_send_doc(struct context *context, struct conn *conn, struct msg *msg, fdb_doc *doc);
struct msg *msg_get(struct conn *conn, bool request);
void msg_put(struct context *ctx, struct msg *msg);
struct msg *rsp_send_next(struct context *context, struct conn *conn);
bool msg_empty(struct msg *msg);
void req_put(struct context *ctx, struct msg *msg);
void rsp_put(struct context *ctx, struct msg *msg);
void req_recv_done(struct context *ctx, struct conn *conn, struct msg *msg, struct msg *nmsg);
void rsp_send_done(struct context *ctx, struct conn *conn, struct msg *msg);
void req_process_error(struct context *ctx, struct conn *conn, struct msg *msg, int err);
bool req_done(struct conn *conn, struct msg *msg);
void rsp_send_status(struct context *ctx, struct conn *conn, struct msg *msg, msg_type_t rsp_type);
#endif
