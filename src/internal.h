
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
#include <assert.h>
#include <string.h>

#include "uv.h"
#include "http_parser.h"
#include "squirrel_util.h"
#include "squirrel_string.h"
#include "squirrel_pool.h"
#include "squirrel_http.h"
#include "squirrel_atomic.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct shttp_listening_s {
  shttp_t                                 *http;
  uv_tcp_t                                uv_handle;

  sstring_t                               net;
  sstring_t                               address;

  TAILQ_ENTRY(shttp_listening_s)          next;
} shttp_listening_t;

TAILQ_HEAD(shttp_listenings, shttp_listening_s);
typedef struct shttp_listenings shttp_listenings_t;


#define SHTTP_REQUEST_STATUS_DEF(XX)                           \
  XX(none,        0)                                           \
  XX(parse_begin,       1)                                     \
  XX(parse_status,      2)                                     \
  XX(parse_url,         3)                                     \
  XX(parse_field,       4)                                     \
  XX(parse_value,       5)                                     \
  XX(parse_headers_ok,  6)                                     \
  XX(parse_body,        7)                                     \
  XX(parse_end,         8)                                     \
  XX(failed,            254)

typedef enum shttp_request_status_e {
#define XX(name, value)     shttp_request_##name  = value,
  SHTTP_REQUEST_STATUS_DEF(XX)
#undef  XX
  shttp_request_unknown                   = 255
} shttp_request_status_t;


#define SHTTP_CONNECTION_STATUS_DEF(XX)                        \
  XX(none,                 0)                                  \
  XX(request_reading,      1)                                  \
  XX(request_parsing,      2)                                  \
  XX(response_creating,    3)                                  \
  XX(response_writing,     4)                                  \
  XX(closing,              5)                                  \
  XX(failed,             254)

typedef enum shttp_connection_status_e {
#define XX(name, value)     shttp_connection_##name  = value,
  SHTTP_CONNECTION_STATUS_DEF(XX)
#undef  XX
  shttp_connection_unknown                   = 255
} shttp_connection_status_t;

#define shttp_connection_is_response_processing(status)        \
   (shttp_connection_response_creating == (status) ||          \
   shttp_connection_response_writing == (status))

#define shttp_headers_is_reading(conn)                         \
   (shttp_request_parse_headers_ok > conn_incomming(conn).status)


#define shttp_write_buffers_size          32
#define shttp_head_write_buffers_size     shttp_write_buffers_size
#define shttp_body_write_buffers_size     shttp_write_buffers_size
#define shttp_write_cb_buffers_size       (2*shttp_write_buffers_size)
#define shttp_set_length(s, l)               (s).length = (l)

typedef struct shttp_buffer_s {
  char                                  * s;
  size_t                                  base;
  size_t                                  start;
  size_t                                  end;
  size_t                                  capacity;
} shttp_buffer_t;

typedef struct shttp_kv_buffer_s {
  size_t                                   capacity;
  shttp_kv_t                             * array;
} shttp_kv_buffer_t;

typedef struct shttp_buffers_s {
  size_t                                   capacity;
  size_t                                   length;
  uv_buf_t                                 array[shttp_write_buffers_size];
} shttp_buffers_t;

typedef struct shttp_connection_cb_buffer_s {
  size_t                                   capacity;
  size_t                                   length;
  shttp_connection_cb_t                    array[shttp_write_cb_buffers_size];
} shttp_connection_cb_buffer_t;

typedef struct shttp_context_cb_buffer_s {
  size_t                                   capacity;
  size_t                                   length;
  shttp_context_cb_t                       array[shttp_write_cb_buffers_size];
} shttp_context_cb_buffer_t;

typedef struct shttp_incomming_s {
  // should init while creating
  shttp_kv_buffer_t                        headers;

  // should init while new request is arrived
  shttp_request_t                          request;
  shttp_request_status_t                   status;
  void                                   * headers_block;
  sstring_t                                headers_buf;
  cstring_t                                url;
} shttp_incomming_t;

typedef struct shttp_outgoing_s {
  // should init while creating
  shttp_kv_buffer_t                        headers;
  shttp_buffers_t                          head_write_buffers;
  shttp_buffers_t                          body_write_buffers;
  shttp_context_cb_buffer_t                call_after_completed;
  shttp_context_cb_buffer_t                call_after_data_writed;

  // should init while new request is arrived
  shttp_response_t                         response;
  uv_write_t                               write_req;
  sstring_t                                content_type;
  sbuffer_t                                head_buffer;
  sbuffer_t                                body_buffer;
  shttp_res                                failed_code;
  uint32_t                                 is_body_end:1;
  uint32_t                                 head_writed:1;
#ifdef SHTTP_THREAD_SAFE
  uint32_t                                 reserved1:30;
  atomic32_t                               is_async;
  atomic32_t                               is_writing;
  atomic32_t                               thread_id;
#else
  uint32_t                                 is_async:1;
  uint32_t                                 is_writing:1;
  uint32_t                                 reserved1:28;
  uint32_t                                 thread_id;
#endif
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

#define ctx_request(it) (it)->incomming.request
#define ctx_response(it) (it)->outgoing.response
#define ctx_incomming(it) (it)->incomming
#define ctx_outgoing(it) (it)->outgoing

TAILQ_HEAD(shttp_contexts_s, shttp_context_internal_s);
typedef struct shttp_contexts_s shttp_contexts_t;

#ifdef SHTTP_THREAD_SAFE
#define shttp_atomic_add32(mem, val)               atomic_add32(mem, val)
#define shttp_atomic_sub32(mem, val)               atomic_sub32(mem, val)
#define shttp_atomic_dec32(mem)                    atomic_dec32(mem)
#define shttp_atomic_inc32(mem)                    atomic_inc32(mem)
#define shttp_atomic_read32(mem)                   atomic_read32(mem)
#define shttp_atomic_set32(mem, val)               atomic_set32(mem, val)
#define shttp_atomic_cvs32(mem, new_val, old_val)  atomic_cvs32(mem, new_val, old_val)
#else
#define shttp_atomic_add32(mem, val)               *(mem) += val
#define shttp_atomic_sub32(mem, val)               *(mem) -= val
#define shttp_atomic_dec32(mem)                    -- *(mem)
#define shttp_atomic_inc32(mem)                    ++ *(mem)
#define shttp_atomic_read32(mem)                   *(mem)
#define shttp_atomic_set32(mem, val)               *(mem) = val
#define shttp_atomic_cvs32(mem, new_val, old_val)  *(mem) = ((*(mem) == (old_val))? (new_val) : (old_val))
#endif

typedef struct shttp_connection_internal_s {
  // should init while creating
  shttp_connection_t                       external;
  shttp_callbacks_t                      * callbacks;
  spool_t                                  pool;
  void                                   * arena_base;
  size_t                                   arena_capacity;


  // should init while new connection is arrived
  shttp_connection_status_t                status;
  uv_async_t                               flush_signal;
  uv_tcp_t                                 uv_handle;
  struct http_parser                       parser;
  shttp_buffer_t                           rd_buf;
  shttp_context_internal_t               * current_ctx;
  int                                      context_id;

  TAILQ_ENTRY(shttp_connection_internal_s) next;
} shttp_connection_internal_t;


#define conn_external(it)  (it)->external
#define conn_request(it)   (it)->current_ctx->incomming.request
#define conn_response(it)  (it)->current_ctx->outgoing.response

#define conn_incomming(it) (it)->current_ctx->incomming
#define conn_outgoing(it)  (it)->current_ctx->outgoing



TAILQ_HEAD(shttp_connections_s, shttp_connection_internal_s);
typedef struct shttp_connections_s shttp_connections_t;

struct shttp_s {
  uv_loop_t                             * uv_loop;
  uv_async_t                              shutdown_signal;
  struct http_parser_settings             parser_settings;

  shttp_connections_t                     connections;
  shttp_connections_t                     free_connections;

  shttp_contexts_t                        free_contexts;

  shttp_listenings_t                      listenings;
  shttp_settings_t                        settings;

  void                                  * context;
};


#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))

#define SHTTP_STATUS_OK                         200

#define UV_CHECK(r, loop, msg)                                                  \
    if (r) {                                                                    \
        ERR("%s: %s\n", msg, uv_strerror(r));                                   \
        return SHTTP_RES_UV;                                                    \
    }



static void _shttp_connection_on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf);
static void _shttp_connection_on_alloc(uv_handle_t* req, size_t suggested_size, uv_buf_t* buf);

void _shttp_connection_restart_read_request(shttp_connection_internal_t* conn);
void _shttp_connection_on_data_writed(uv_write_t* req, int status);
void _shttp_connection_on_head_writed(uv_write_t* req, int status);
void _shttp_connection_on_disconnect(uv_handle_t* handle);
int  _shttp_connection_on_message_begin(shttp_connection_internal_t*);
int  _shttp_connection_on_headers_complete(shttp_connection_internal_t* conn);
int  _shttp_connection_on_body (shttp_connection_internal_t*, const char *at, size_t length);
int  _shttp_connection_on_message_complete (shttp_connection_internal_t*);


#define call_for(ctx, action)                                                         \
  size_t                     i;                                                       \
  shttp_context_cb_buffer_t  cb_buf;                                                  \
                                                                                      \
  memcpy(&cb_buf.array[0],  &ctx_outgoing(ctx).call_after_##action.array[0],          \
    sizeof(shttp_context_cb_t)*ctx_outgoing(ctx).call_after_##action.length);         \
  cb_buf.length = ctx_outgoing(ctx).call_after_##action.length;                       \
  ctx_outgoing(ctx).call_after_##action.length = 0;                                   \
                                                                                      \
  for(i = 0; i < cb_buf.length; i ++) {                                               \
    cb_buf.array[i].cb(&ctx->external, cb_buf.array[i].data);                         \
  }

static inline void _shttp_response_call_hooks_after_completed(shttp_context_internal_t *ctx) {
  call_for(ctx, completed);
}

static inline void _shttp_response_call_hooks_after_writed(shttp_context_internal_t *ctx) {
  call_for(ctx, data_writed);
}


typedef void (*swrite_cb_t)(shttp_connection_internal_t*);
void _shttp_response_async_send(shttp_connection_internal_t *conn);
void _shttp_response_sync_send(shttp_connection_internal_t *conn);
void _shttp_response_send_error_message(shttp_connection_internal_t *conn,
                                        const char* message_str,
                                        size_t message_len);
void _shttp_response_send_error_message_format(shttp_connection_internal_t *conn,
    const char  *fmt,
    ...);
void _shttp_response_on_completed(shttp_connection_internal_t *conn, int status);

#ifdef DEBUG
static inline void _shttp_response_assert_after_response_end(shttp_connection_t *external) {
  shttp_connection_internal_t    *conn;
  conn = (shttp_connection_internal_t*) external->internal;
  shttp_assert(0 == conn_outgoing(conn).head_write_buffers.length);
  shttp_assert(0 == conn_outgoing(conn).body_write_buffers.length);
  shttp_assert(0 == conn_outgoing(conn).call_after_data_writed.length);
  shttp_assert(0 == conn_outgoing(conn).call_after_completed.length);
}
#else
#define _shttp_response_assert_after_response_end(conn)
#endif



#define shttp_strerr(num) num



#define INTERNAL_ERROR_HEADERS(NUM)   "HTTP/1.1 " NUM "\r\n"                          \
                                 "Connection: close\r\n"                              \
                                 "Content-Type: text/plain\r\n"                       \
                                 "\r\n"


#define BODY_NOT_COMPLETE        INTERNAL_ERROR_HEADERS("500 Internal Server Error")  \
                                 "Internal Server Error:\r\n"                         \
                                 "\tBODY_NOT_COMPLETE"


#define INTERNAL_ERROR_FROMAT    INTERNAL_ERROR_HEADERS("500 Internal Server Error")  \
                                 "Internal Server Error:\r\n"                         \
                                 "\t%d"


const char * _shttp_connection_status_text(shttp_connection_status_t status);
const char * _shttp_request_status_text(shttp_request_status_t status);
void _shttp_status_init();
int stats_handler(shttp_context_t* external);

#ifdef __cplusplus
}
#endif

#endif //_INTERNAL_H_
