
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
#include "squirrel_slab.h"
#include "squirrel_http.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct shttp_listening_s {
  shttp_t    *http;
  uv_tcp_t    uv_handle;

  sstring_t   net;
  sstring_t   address;

  TAILQ_ENTRY(shttp_listening_s) next;
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


typedef struct shttp_kv_array_s {
  size_t      capacity;
  size_t      length;
  shttp_kv_t  *array;
} shttp_kv_array_t;

typedef struct shttp_incomming_s {
  cstring_t                          url;
  shttp_kv_array_t                   headers;
  struct shttp_connection_internal_s *conn;
  
  shttp_message_status_t             status;
  int                                head_reading;
  struct http_parser                 parser;
  size_t                             headers_bytes_count;
} shttp_incomming_t;

typedef struct shttp_send_buffer_s {
  size_t      capacity;
  size_t      length;
  shttp_kv_t  array[256];
} shttp_send_buffer_t;

typedef struct shttp_outgoing_s {
  shttp_send_buffer_t buffer;
  shttp_header_t      headers;
} shttp_outgoing_t;

typedef struct shttp_connection_internal_s {
  uv_tcp_t                uv_handle;
  shttp_connection_t      inner;
  shttp_callbacks_t       *callbacks;
  shttp_incomming_t       incomming;
  shttp_outgoing_t        outgoing;
  shttp_slab_pool_t*      pool;
  void*                   arena_base;
  size_t                  arena_capacity;
  size_t                  arena_offset;

  TAILQ_ENTRY(shttp_connection_internal_s) next;
} shttp_connection_internal_t;

TAILQ_HEAD(shttp_connections_s, shttp_connection_internal_s);
typedef struct shttp_connections_s shttp_connections_t;

struct shttp_s {
  uv_loop_t*                     uv_loop;
  struct http_parser_settings    parser_settings;

  shttp_connections_t            connections;
  shttp_connections_t            free_connections;

  shttp_listenings_t             listenings;
  shttp_settings_t               settings;
};

#define UV_CHECK(r, loop, msg)                    \
    if (r) {                                      \
        ERR("%s: %s\n", msg, uv_strerror(r));     \
        return SHTTP_RES_UV;                      \
    }

#define LOG_HTTP(at, len, fmt, ...) _shttp_log_data(at, len, fmt "\n", __VA_ARGS__);
#define LOG(level, ...) _shttp_log_printf(level, __VA_ARGS__)
#define TRACE(...) _shttp_log_printf(SHTTP_LOG_DEBUG, __VA_ARGS__)
#define WARN(...)  _shttp_log_printf(SHTTP_LOG_WARN, __VA_ARGS__)
#define ERR(...) _shttp_log_printf(SHTTP_LOG_ERR, __VA_ARGS__)
#define CRIT(...)  _shttp_log_printf(SHTTP_LOG_CRIT, __VA_ARGS__)

// Log Functions
void _shttp_log_data(const char* data, size_t len, const char* fmt, ...);
extern void _shttp_log_printf(int severity, const char *fmt, ...);



extern int shttp_win32_version;
extern int shttp_ncpu;
extern int shttp_cacheline_size;
extern int shttp_pagesize;
extern int shttp_pagesize_shift;
extern void os_init();

#ifdef __cplusplus
}
#endif

#endif //_INTERNAL_H_
