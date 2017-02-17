#ifndef _GAIA_FORESTDB_H_
#define _GAIA_FORESTDB_H_

#define forestdb_free(_p) free(_p)
#define forestdb_doc_free(_d) fdb_doc_free(_d)

int forestdb_init(struct context *ctx);
int forestdb_set(struct context *ctx, uint8_t *key, size_t klen, uint8_t *value, size_t vlen, uint32_t flags, uint32_t expiry);
fdb_doc *forestdb_get(struct context *ctx, uint8_t *key, size_t klen);
int forestdb_del();
#endif
