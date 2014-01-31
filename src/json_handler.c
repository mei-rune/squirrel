#include "squirrel_config.h"
#include "squirrel_http.h"
#include "yajl/yajl_common.h"
#include "yajl/yajl_parse.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct json_context_s {
  yajl_handle         handle;
  int                 skip_parse;
} json_context_t;


void* json_malloc(void *ctx, size_t sz) {
  return shttp_response_mem_malloc((shttp_connection_t*)ctx, sz);
}

void json_free(void *ctx, void * ptr) {
  return shttp_response_mem_free((shttp_connection_t*)ctx, ptr);
}

void* json_realloc(void *ctx, void * ptr, size_t sz) {
  return shttp_response_mem_realloc((shttp_connection_t*)ctx, ptr, sz);
}

void* json_init(shttp_connection_t* conn, const yajl_callbacks * callbacks, void *ctx) {
  yajl_alloc_funcs funcs;
  funcs.malloc = &json_malloc;
  funcs.realloc = &json_realloc;
  funcs.free = &json_free;
  funcs.ctx = conn;

  return yajl_alloc(callbacks, &funcs, ctx);
}

//int json_on_body(shttp_connection_t* conn, const char *at, size_t length) {
//  json_context_t    *ctx;
//  yajl_status       status;
//
//  ctx = (json_context_t*)conn->ctx;
//
//  switch(yajl_parse(ctx->handle, (const unsigned char*) at, length)) {
//  case yajl_status_ok:
//    return 0;
//  case yajl_status_client_canceled:
//    return -1;
//
//  }
//}

#ifdef __cplusplus
};
#endif