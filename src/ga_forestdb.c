#include "ga_core.h"

int forestdb_init(struct context *ctx) {
    fdb_status status;
    ctx->config = fdb_get_default_config();
    ctx->config.durability_opt = FDB_DRB_ASYNC;
    ctx->config.compaction_mode = FDB_COMPACTION_AUTO;

    status = fdb_init(&ctx->config);
    if (status != FDB_RESULT_SUCCESS) {
        ga_log(ERROR, "fdb init failed: %d", status);
        return status;
    }

    status = fdb_open(&ctx->fhandle, "/tmp/gaia", &ctx->config);
    if (status != FDB_RESULT_SUCCESS) {
        ga_log(ERROR, "fdb open failed: %d", status);
        return status;
    }

    status = fdb_kvs_open_default(ctx->fhandle, &ctx->kvhandle, &ctx->kvs_config);

    return status == FDB_RESULT_SUCCESS ? GA_OK : GA_ERR;
}

int forestdb_set(struct context *ctx, uint8_t *key, size_t klen, uint8_t *value, size_t vlen, uint32_t flags, uint32_t expiry) {
    fdb_status status;
    fdb_doc *doc;

    ASSERT(ctx->kvhandle);
    ASSERT(ctx->fhandle);

    int flen;
    char buf[GA_UINTMAX_MAXLEN + 1];

    flen = sprintf(buf, "%d:%d", flags, expiry);
    status = fdb_doc_create(&doc, key, klen, buf, flen, value, vlen);
    if (status != FDB_RESULT_SUCCESS) {
        ga_log(ERROR, "fdb doc create failed: %d", status);
        return GA_ERR;
    }

    status = fdb_set(ctx->kvhandle, doc);
    if (status != FDB_RESULT_SUCCESS) {
        ga_log(ERROR, "fdb set failed: %d", status);
        return GA_ERR;
    }

    status = fdb_commit(ctx->fhandle, FDB_COMMIT_NORMAL);
    if (status != FDB_RESULT_SUCCESS) {
        ga_log(ERROR, "fdb commit faield: %d", status);
        return GA_ERR;
    }

    status = fdb_doc_free(doc);
    if (status != FDB_RESULT_SUCCESS) {
        ga_log(ERROR, "fdb doc free faield: %d", status);
        return GA_OK;
    }

    return GA_OK;
}

fdb_doc *forestdb_get(struct context *ctx, uint8_t *key, size_t klen) {
    fdb_status status;
    fdb_doc *doc;

    ASSERT(ctx->kvhandle);

    status = fdb_doc_create(&doc, key, klen, NULL, 0, NULL, 0);
    if (status != FDB_RESULT_SUCCESS) {
        ga_log(ERROR, "fdb doc create faield: %d", status);
        return NULL;
    }

    status = fdb_get(ctx->kvhandle, doc);
    if (status != FDB_RESULT_SUCCESS) {
        if (status == FDB_RESULT_KEY_NOT_FOUND)
            return NULL;
        ga_log(ERROR, "fdb doc get failed: %d", status);
        return NULL;
    }

    return doc;
}

int forestdb_update() { return GA_OK; }

int forestdb_del() { return GA_OK; }
