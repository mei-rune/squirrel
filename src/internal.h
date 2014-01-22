
#ifndef _HTTP_INTERNAL_H_
#define _HTTP_INTERNAL_H_

#include "squirrel_config.h"

#ifdef _MSC_VER
#include <BaseTsd.h>
#include "sys/queue.h"
#else
#include <sys/queue.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#ifndef _MSC_VER
#include <inttypes.h>
#include <stdbool.h>
#endif
#include <string.h>

#include "uv.h"
#include "http_parser.h"
#include "squirrel_util.h"
#include "squirrel_string.h"
#include "squirrel_pool.h"
#include "squirrel_http.h"


#ifdef __cplusplus
extern "C" {
#endif
  


typedef struct shttp_listening_s {
  shttp_t                             *http;
  uv_tcp_t                            uv_handle;

  sstring_t                           net;
  sstring_t                           address;

  TAILQ_ENTRY(shttp_listening_s)      next;
} shttp_listening_t;

TAILQ_HEAD(shttp_listenings, shttp_listening_s);
typedef struct shttp_listenings shttp_listenings_t;


typedef enum shttp_message_status_e {
  shttp_message_begin   = 0,
  shttp_message_status  = 1,
  shttp_message_url     = 2,
  shttp_message_field   = 3,
  shttp_message_value   = 4,
  shttp_message_body    = 5,
  shttp_message_end     = 6
} shttp_message_status_t;

typedef struct shttp_connection_internal_s {
  shttp_contexts_t                        contexts;
  uv_tcp_t                                uv_handle;
  shttp_connection_t                      external;
  shttp_context_internal_t                *current_ctx;
  shttp_callbacks_t                       *callbacks;
  spool_t                                 pool;

  
  // should init while new connection is arrived
  struct http_parser                      parser;
  uint16_t                                status;
  uint16_t                                head_reading;
  uint32_t                                headers_bytes_count;

  TAILQ_ENTRY(shttp_connection_internal_s) next;
} shttp_connection_internal_t;

#define conn_external(it) (it)->external

TAILQ_HEAD(shttp_connections_s, shttp_connection_internal_s);
typedef struct shttp_connections_s shttp_connections_t;


#define shttp_write_buffers_size    32
#define shttp_head_write_buffers_size shttp_write_buffers_size
#define shttp_body_write_buffers_size shttp_write_buffers_size
#define shttp_write_cb_buffers_size (2*shttp_write_buffers_size)

typedef struct shttp_kv_array_s {
  size_t                               capacity;
  size_t                               length;
  shttp_kv_t                           *array;
} shttp_kv_array_t;

typedef struct shttp_kv_buffer_s {
  size_t                               capacity;
  shttp_kv_t                           *array;
} shttp_kv_buffer_t;

typedef struct shttp_buffers_s {
  size_t                               capacity;
  size_t                               length;
  uv_buf_t                             array[shttp_write_buffers_size];
} shttp_buffers_t;

typedef struct shttp_write_cb_buffer_s {
  size_t                               capacity;
  size_t                               length;
  shttp_ctx_write_cb_t                 array[shttp_write_cb_buffers_size];
} shttp_write_cb_buffer_t;


typedef struct shttp_incomming_s {
  // should init while creating
  shttp_kv_array_t                      headers;
  
} shttp_incomming_t;

typedef struct shttp_outgoing_s {
  // should init while creating
  shttp_kv_buffer_t                     headers;
  shttp_buffers_t                       head_write_buffers;
  shttp_buffers_t                       body_write_buffers;
  shttp_write_cb_buffer_t               call_after_completed;
  shttp_write_cb_buffer_t               call_after_data_writed;
  
  // should init while new request is arrived
  uv_write_t                            write_req;
  sstring_t                             content_type;
  sbuffer_t                             head_buffer;
  shttp_res                             failed_code;
  uint64_t                              is_body_end:1;
  uint64_t                              reserved:63;
} shttp_outgoing_t;

typedef struct shttp_context_internal_s {
  shttp_context_t                       external;
  shttp_incomming_t                     incomming;
  shttp_outgoing_t                      outgoing;
  
  void*                                 arena_base;
  size_t                                arena_capacity;
  spool_t                               pool;

  TAILQ_ENTRY(shttp_context_internal_s) next;
} shttp_context_internal_t;

#define ctx_request(it) (it)->external.request
#define ctx_response(it) (it)->external.response
#define ctx_incomming(it) (it)->incomming
#define ctx_outgoing(it) (it)->outgoing

TAILQ_HEAD(shttp_contexts_s, shttp_context_internal_s);
typedef struct shttp_contexts_s shttp_contexts_t;

struct shttp_s {
  uv_loop_t                      *uv_loop;
  struct http_parser_settings    parser_settings;

  shttp_connections_t            connections;
  shttp_connections_t            free_connections;

  shttp_contexts_t               free_contexts;

  shttp_listenings_t             listenings;
  shttp_settings_t               settings;
};

#define UV_CHECK(r, loop, msg)                    \
    if (r) {                                      \
        ERR("%s: %s\n", msg, uv_strerror(r));     \
        return SHTTP_RES_UV;                      \
    }

#define SHTTP_STATUS_OK  200


static inline void _shttp_connection_assert_ctx(shttp_t* http, shttp_context_internal_t* ctx) {
  assert(((char*)ctx_outgoing(ctx).headers.array) == (((char*)ctx) + shttp_mem_align(sizeof(shttp_connection_internal_t)) +
         http->settings.context_user_ctx_size +
         (sizeof(shttp_kv_t) * http->settings.max_headers_count)));
  assert(ctx_outgoing(ctx).headers.capacity == http->settings.max_headers_count);

  assert(shttp_head_write_buffers_size == ctx_outgoing(ctx).head_write_buffers.capacity);
  assert(shttp_body_write_buffers_size == ctx_outgoing(ctx).body_write_buffers.capacity);
  assert(shttp_write_cb_buffers_size == ctx_outgoing(ctx).call_after_data_writed.capacity);
  assert(shttp_write_cb_buffers_size == ctx_outgoing(ctx).call_after_completed.capacity);
  
  
  assert(ctx_outgoing(ctx).content_type.len == 0);
  assert(ctx_outgoing(ctx).content_type.str == nil);
  assert(ctx_outgoing(ctx).head_buffer.len == 0);
  assert(ctx_outgoing(ctx).head_buffer.str == nil);
  assert(ctx_outgoing(ctx).head_buffer.capacity == 0);
}

static inline shttp_context_internal_t* _shttp_connection_get_new_ctx(shttp_t* http) {
  shttp_context_internal_t* ctx;
  ctx = TAILQ_FIRST(&http->free_contexts);
  if(nil == ctx) {
    return nil;
  }

  memset(&ctx_response(ctx), 0, sizeof(shttp_response_t));
  memset(&ctx_request(ctx), 0, sizeof(shttp_request_t));

  _shttp_connection_assert_ctx(http, ctx);

  ctx_outgoing(ctx).write_req.data = nil;
  ctx_outgoing(ctx).head_write_buffers.length = 0;
  ctx_outgoing(ctx).body_write_buffers.length = 0;
  ctx_outgoing(ctx).call_after_completed.length = 0;
  ctx_outgoing(ctx).call_after_data_writed.length = 0;
  ctx_outgoing(ctx).is_body_end = 0;
  ctx_outgoing(ctx).failed_code = 0;
  ctx_outgoing(ctx).reserved = 0;



  ctx_response(ctx).headers.array = ctx_outgoing(ctx).headers.array;
  ctx_response(ctx).status_code = SHTTP_STATUS_OK;
  ctx_response(ctx).chunked = SHTTP_THUNKED_NONE;
  ctx_response(ctx).close_connection = SHTTP_CLOSE_CONNECTION_NONE;

  TAILQ_REMOVE(&http->free_contexts, ctx, next);
  return ctx;
}


static inline void _shttp_connection_free_ctx(shttp_t* http, shttp_context_internal_t* ctx) {
  _shttp_connection_assert_ctx(http, ctx);
  TAILQ_INSERT_TAIL(&http->free_contexts, ctx, next);
}


static inline void _shttp_response_call_hooks_after_writed(shttp_context_internal_t *ctx) {
  int                     i;
  shttp_write_cb_buffer_t cb_buf;

  memcpy(&cb_buf.array[0],  &ctx_outgoing(ctx).call_after_data_writed.array[0], 
    sizeof(shttp_ctx_write_cb_t)*ctx_outgoing(ctx).call_after_data_writed.length);
  cb_buf.length = ctx_outgoing(ctx).call_after_data_writed.length;

  ctx_outgoing(ctx).call_after_data_writed.length = 0;
  for(i = 0; i < cb_buf.length; i ++) {
    cb_buf.array[i].cb(&conn->inner, cb_buf.array[i].data);
  }
}

void _shttp_response_send_ready_data(shttp_connection_internal_t *conn,
                                     uv_close_cb on_disconnect,
                                     uv_write_cb on_head_writed,
                                     uv_write_cb on_writed);
void _shttp_response_send_error_message(shttp_connection_internal_t *conn,
                                        uv_close_cb on_disconnect,
                                         const char* message_str,
                                         size_t message_len);
void _shttp_response_send_error_message_format(shttp_connection_internal_t *conn,
                                        uv_close_cb on_disconnect,
                                        const char  *fmt,
                                        ...);




#define shttp_strerr(num) num


#define INTERNAL_ERROR_HEADERS   "HTTP/0.0 500 Internal Server Error\r\n"    \
                                 "Connection: close\r\n"                     \
                                 "Content-Type: text/plain\r\n"              \
                                 "\r\n"


#define BODY_NOT_COMPLETE        INTERNAL_ERROR_HEADERS                      \
                                 "Internal Server Error:\r\n"                \
                                 "\tBODY_NOT_COMPLETE"


#define INTERNAL_ERROR_FROMAT    INTERNAL_ERROR_HEADERS                      \
                                 "Internal Server Error:\r\n"                \
                                 "\t%d"


#ifdef __cplusplus
}
#endif

#endif //_INTERNAL_H_
