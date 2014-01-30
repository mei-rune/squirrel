
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


typedef enum shttp_message_request_parse_status_e {
  shttp_message_request_parse_none        = 0,
  shttp_message_request_parse_begin       = 1,
  shttp_message_request_parse_status      = 2,
  shttp_message_request_parse_url         = 3,
  shttp_message_request_parse_field       = 4,
  shttp_message_request_parse_value       = 5,
  shttp_message_request_parse_headers_ok  = 6,
  shttp_message_request_parse_body        = 7,
  shttp_message_request_parse_end         = 8,
  shttp_message_response_begin            = 9,
  shttp_message_response_writing          = 10,
  shttp_message_response_end              = 11,

  shttp_message_failed                    = 255
} shttp_message_request_parse_status_t;


#define shttp_headers_is_reading(conn)  (shttp_message_request_parse_headers_ok > conn->status)


#define shttp_write_buffers_size          32
#define shttp_head_write_buffers_size     shttp_write_buffers_size
#define shttp_body_write_buffers_size     shttp_write_buffers_size
#define shttp_write_cb_buffers_size       (2*shttp_write_buffers_size)
#define shttp_set_length(s, l)               (s).length = (l)

typedef struct shttp_buffer_s {
  char                                    *s;
  size_t                                  base;
  size_t                                  start;
  size_t                                  end;
  size_t                                  capacity;
} shttp_buffer_t;

typedef struct shttp_kv_array_s {
  size_t                                  capacity;
  size_t                                  length;
  shttp_kv_t                              *array;
} shttp_kv_array_t;

typedef struct shttp_kv_buffer_s {
  size_t                                   capacity;
  shttp_kv_t                               *array;
} shttp_kv_buffer_t;

typedef struct shttp_buffers_s {
  size_t                                   capacity;
  size_t                                   length;
  uv_buf_t                                 array[shttp_write_buffers_size];
} shttp_buffers_t;

typedef struct shttp_write_cb_buffer_s {
  size_t                                   capacity;
  size_t                                   length;
  shttp_write_cb_t                         array[shttp_write_cb_buffers_size];
} shttp_write_cb_buffer_t;

typedef struct shttp_incomming_s {
  // should init while creating
  shttp_kv_array_t                         headers;

  // should init while new request is arrived
  void                                     *headers_block;
  sstring_t                                headers_buf;
  cstring_t                                url;
} shttp_incomming_t;

typedef struct shttp_outgoing_s {
  // should init while creating
  shttp_kv_buffer_t                        headers;
  shttp_buffers_t                          head_write_buffers;
  shttp_buffers_t                          body_write_buffers;
  shttp_write_cb_buffer_t                  call_after_completed;
  shttp_write_cb_buffer_t                  call_after_data_writed;


  // should init while new request is arrived
  uv_write_t                               write_req;
  sstring_t                                content_type;
  sbuffer_t                                head_buffer;
  shttp_res                                failed_code;
  uint64_t                                 is_body_end:1;
  uint64_t                                 reserved:63;
} shttp_outgoing_t;

typedef struct shttp_connection_internal_s {
  // should init while creating
  shttp_connection_t                       external;
  shttp_callbacks_t                        *callbacks;
  shttp_incomming_t                        incomming;
  shttp_outgoing_t                         outgoing;
  spool_t                                  pool;
  void                                     *arena_base;
  size_t                                   arena_capacity;


  // should init while new connection is arrived
  uv_tcp_t                                 uv_handle;
  shttp_message_request_parse_status_t     status;
  struct http_parser                       parser;
  shttp_buffer_t                           rd_buf;

  TAILQ_ENTRY(shttp_connection_internal_s) next;
} shttp_connection_internal_t;


#define conn_external(it)  (it)->external
#define conn_request(it)   (it)->external.request
#define conn_response(it)  (it)->external.response
#define conn_incomming(it) (it)->incomming
#define conn_outgoing(it)  (it)->outgoing



TAILQ_HEAD(shttp_connections_s, shttp_connection_internal_s);
typedef struct shttp_connections_s shttp_connections_t;

struct shttp_s {
  uv_loop_t                      *uv_loop;
  struct http_parser_settings    parser_settings;

  shttp_connections_t            connections;
  shttp_connections_t            free_connections;

  shttp_listenings_t             listenings;
  shttp_settings_t               settings;
};

#define SHTTP_STATUS_OK                         200

#define UV_CHECK(r, loop, msg)                                                  \
    if (r) {                                                                    \
        ERR("%s: %s\n", msg, uv_strerror(r));                                   \
        return SHTTP_RES_UV;                                                    \
    }



static void _shttp_connection_on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf);
static void _shttp_connection_on_alloc(uv_handle_t* req, size_t suggested_size, uv_buf_t* buf);
static void _shttp_connection_on_data_writed(uv_write_t* req, int status);
static void _shttp_connection_on_head_writed(uv_write_t* req, int status);
static void _shttp_connection_on_disconnect(uv_handle_t* handle);
int  _shttp_connection_on_message_begin(shttp_connection_internal_t*);
int  _shttp_connection_on_headers_complete(shttp_connection_internal_t* conn);
int  _shttp_connection_on_body (shttp_connection_internal_t*, const char *at, size_t length);
int  _shttp_connection_on_message_complete (shttp_connection_internal_t*);


#define call_for(conn, action)                                                  \
  size_t                     i;                                                 \
  shttp_write_cb_buffer_t cb_buf;                                               \
                                                                                \
  memcpy(&cb_buf.array[0],  &conn_outgoing(conn).call_after_##action.array[0],  \
    sizeof(shttp_write_cb_t)*conn_outgoing(conn).call_after_##action.length);   \
  cb_buf.length = conn_outgoing(conn).call_after_##action.length;               \
  conn_outgoing(conn).call_after_##action.length = 0;                           \
                                                                                \
  for(i = 0; i < cb_buf.length; i ++) {                                         \
    cb_buf.array[i].cb(&conn_external(conn), cb_buf.array[i].data);             \
  }

static inline void _shttp_response_call_hooks_after_completed(shttp_connection_internal_t *conn) {
  call_for(conn, completed);
}

static inline void _shttp_response_call_hooks_after_writed(shttp_connection_internal_t *conn) {
  call_for(conn, data_writed);
}

void _shttp_response_send_ready_data(shttp_connection_internal_t *conn,
                                     uv_close_cb on_disconnect,
                                     uv_write_cb on_head_writed,
                                     uv_write_cb on_data_writed);
void _shttp_response_send_error_message(shttp_connection_internal_t *conn,
                                        uv_close_cb on_disconnect,
                                        const char* message_str,
                                        size_t message_len);
void _shttp_response_send_error_message_format(shttp_connection_internal_t *conn,
    uv_close_cb on_disconnect,
    const char  *fmt,
    ...);

#ifdef DEBUG
static inline void _shttp_response_assert_after_response_end(shttp_connection_internal_t *conn) {
  assert(0 == conn_outgoing(conn).head_write_buffers.length);
  assert(0 == conn_outgoing(conn).body_write_buffers.length);
  assert(0 == conn_outgoing(conn).call_after_data_writed.length);
  assert(0 == conn_outgoing(conn).call_after_completed.length);
}
#else
#define _shttp_response_assert_after_response_end(conn)
#endif

static inline void _shttp_response_call_hooks_for_failed(shttp_connection_internal_t *conn) {
  assert(SHTTP_RES_OK != conn_outgoing(conn).failed_code);
  conn_outgoing(conn).head_write_buffers.length = 0;
  conn_outgoing(conn).body_write_buffers.length = 0;

  _shttp_response_call_hooks_after_writed(conn);
  _shttp_response_call_hooks_after_completed(conn);

  _shttp_response_assert_after_response_end(conn);
}



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


void _shttp_status_init();

#ifdef __cplusplus
}
#endif

#endif //_INTERNAL_H_
