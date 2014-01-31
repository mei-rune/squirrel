#ifndef _SHTTP_H_
#define _SHTTP_H_ 1

#include "squirrel_config.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <sys/queue.h>

#include "squirrel_util.h"
#include "squirrel_string.h"
#include "squirrel_pool.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @name Log severities
 */
/**@{*/
#ifndef SHTTP_LOG_DEBUG
#define SHTTP_LOG_DEBUG 0
#endif
#ifndef SHTTP_LOG_WARN
#define SHTTP_LOG_WARN  1
#endif
#ifndef SHTTP_LOG_ERR
#define SHTTP_LOG_ERR   2
#endif
#ifndef SHTTP_LOG_CRIT
#define SHTTP_LOG_CRIT  3
#endif
/**@}*/

/** @name Request Methods
 */
/**@{*/
#define SHTTP_METHOD_MAP(XX)      \
XX(0,  DELETE,      DELETE)       \
XX(1,  GET,         GET)          \
XX(2,  HEAD,        HEAD)         \
XX(3,  POST,        POST)         \
XX(4,  PUT,         PUT)          \
/* pathological */                \
XX(5,  CONNECT,     CONNECT)      \
XX(6,  OPTIONS,     OPTIONS)      \
XX(7,  TRACE,       TRACE)        \
/* webdav */                      \
XX(8,  COPY,        COPY)         \
XX(9,  LOCK,        LOCK)         \
XX(10, MKCOL,       MKCOL)        \
XX(11, MOVE,        MOVE)         \
XX(12, PROPFIND,    PROPFIND)     \
XX(13, PROPPATCH,   PROPPATCH)    \
XX(14, SEARCH,      SEARCH)       \
XX(15, UNLOCK,      UNLOCK)       \
/* subversion */                  \
XX(16, REPORT,      REPORT)       \
XX(17, MKACTIVITY,  MKACTIVITY)   \
XX(18, CHECKOUT,    CHECKOUT)     \
XX(19, MERGE,       MERGE)        \
/* upnp */                        \
XX(20, MSEARCH,     M-SEARCH)     \
XX(21, NOTIFY,      NOTIFY)       \
XX(22, SUBSCRIBE,   SUBSCRIBE)    \
XX(23, UNSUBSCRIBE, UNSUBSCRIBE)  \
/* RFC-5789 */                    \
XX(24, PATCH,       PATCH)        \
XX(25, PURGE,       PURGE)        \
 
enum hw_http_method {
#define XX(num, name, string) SHTTP_REQ_##name = num,
  SHTTP_METHOD_MAP(XX)
#undef XX
};

/**@}*/


#define HTTP_VERSION_1_0   1
#define HTTP_VERSION_1_1   2



typedef struct shttp_write_cb_s        shttp_write_cb_t;
typedef struct shttp_uri_s             shttp_uri_t;
typedef struct shttp_kv_s              shttp_kv_t;
typedef struct shttp_request_s         shttp_request_t;
typedef struct shttp_response_s        shttp_response_t;
typedef struct shttp_callbacks_s       shttp_callbacks_t;
typedef struct http_handler_config_s   shttp_handler_config_t;
typedef struct shttp_connection_s      shttp_connection_t;
typedef struct shttp_settings_s        shttp_settings_t;
typedef struct shttp_s                 shttp_t;
typedef void (*shttp_query_cb)(void* ctx, const char* key, size_t key_len, const char* val, size_t val_len);
typedef int  (*shttp_data_cb) (shttp_connection_t*, const char *at, size_t length);
typedef int  (*shttp_cb) (shttp_connection_t*);
typedef void (*shttp_write_cb) (shttp_connection_t *conn, void *data);


/**
 * @brief structure containing a single callback and configuration
 *
 * The definition structure which is used within the shttp_callbacks_t
 * structure. This holds information during the request processing cycle.
 *
 * Optionally you can set callback-specific hooks just like per-connection
 * hooks using the same rules.
 *
 */
struct shttp_callbacks_s {
  shttp_cb        on_connected;
  shttp_cb        on_message_begin;
  shttp_data_cb   on_body;
  shttp_cb        on_message_complete;
  shttp_cb        on_disconnected;
};



/**
 * the settings of the http server
 */
struct shttp_settings_s {
  /**
   * the timeout for an HTTP request.
   */
  struct timeval timeout;

#define SHTTP_MAX_CONNECTIONS  1000
  /**
   * max connections size, default is SHTTP_MAX_CONNECTIONS
   */
  size_t max_connections_size;

#define SHTTP_CTX_SIZE         256
  /**
   * the user context size per connection, default is SHTTP_CTX_SIZE
   */
  size_t user_ctx_size;

#define SHTTP_MAX_HEADER_COUNT  256
  /**
   * max headers size, default is SHTTP_MAX_HEADER_COUNT
   */
  size_t max_headers_count;

#define SHTTP_MAC_BODY_SIZE    1024*1024*1024
  /**
   * max body bytes, default is SHTTP_MAC_BODY_SIZE
   */
  uint64_t max_body_size;

#define SHTTP_RW_BUFFER_SIZE   200*1024
  /**
   * the read buffer size per connection, default is SHTTP_RW_BUFFER_SIZE
   */
  size_t rw_buffer_size;

  /**
   * Bitmask of all HTTP methods that we accept and pass to user
   * callbacks.
   *
     * If not supported they will generate a "405 Method not allowed" response.
   *
     * By default this includes the following methods: GET, POST, HEAD, PUT, DELETE
   */
  uint16_t allowed_methods;

  /**
   * @brief structure containing a single callback and configuration
   *
   * The definition structure which is used within the evhtp_callbacks_t
   * structure. This holds information about what should execute for either
   * a single or regex path.
   *
   * For example, if you registered a callback to be executed on a request
   * for "/herp/derp", your defined callback will be executed.
   *
   * Optionally you can set callback-specific hooks just like per-connection
   * hooks using the same rules.
   *
   */
  shttp_callbacks_t callbacks;
};

/**
 * @brief a generic container representing an entire URI strucutre
 */
struct shttp_uri_s {
  cstring_t    full;
  cstring_t    schema;
  cstring_t    host;
  cstring_t    port_s;
  uint16_t     port_i;
  cstring_t    path;
  cstring_t    query;
  cstring_t    fragment;
  cstring_t    user_info;
};

/**
 * @brief a generic key/value structure
 */
struct shttp_kv_s {
  cstring_t key;
  cstring_t val;
};


typedef struct shttp_header_s {
  size_t      length;
  shttp_kv_t  *array;
} shttp_header_t;

/**
 * @brief a structure containing all information for a http request.
 */
struct shttp_request_s {
  uint16_t           http_major;
  uint16_t           http_minor;
  uint32_t           method;
  uint64_t           content_length;
  shttp_uri_t        uri;
  shttp_header_t     headers;
};

#define SHTTP_CLOSE_CONNECTION_NONE  0
#define SHTTP_CLOSE_CONNECTION_TRUE  1
#define SHTTP_CLOSE_CONNECTION_FALSE 2

#define SHTTP_THUNKED_NONE           0
#define SHTTP_THUNKED_TRUE           1
#define SHTTP_THUNKED_FALSE          2

/**
 * @brief a structure containing all information for a http request.
 */
struct shttp_response_s {
  uint16_t           http_major;
  uint16_t           http_minor;
  uint32_t           status_code:16;
  uint32_t           close_connection:2;
  uint32_t           chunked:2;
  uint32_t           head_writed:1;
  uint32_t           reserved:11;

  cstring_t          content_type;
  uint64_t           content_length;
  shttp_header_t     headers;
};

struct shttp_connection_s {
  shttp_t           *http;
  shttp_request_t   request;
  shttp_response_t  response;
  spool_t           *pool;
  void              *ctx;
  void              *internal;
};


struct shttp_write_cb_s {
  // Callback info to free buffers
  shttp_write_cb cb;
  void *data;
};

/** @name Response codes
 */
/**@{*/
typedef int                                        shttp_res;
#define SHTTP_RES_OK                                0
#define SHTTP_RES_ERROR                             -1
#define SHTTP_RES_MEMORY                            -2
#define SHTTP_RES_UV                                -3
#define SHTTP_RES_NOTIMPLEMENT                      -4
#define SHTTP_RES_HEAD_WRITED                       -5
#define SHTTP_RES_HEAD_TOO_LARGE                    -6
#define SHTTP_RES_HEAD_TRANSFER_ENCODING            -7
#define SHTTP_RES_FREE_FLAG                         -8
#define SHTTP_RES_INSUFFICIENT_WRITE_CB_BUFFER      -9
#define SHTTP_RES_HEAD_CONNECTION                   -10
#define SHTTP_RES_IN_TRANSACTION                    -11
#define SHTTP_RES_INSUFFICIENT_WRITE_BUFFER         -12
#define SHTTP_RES_HEAD_LINE_TOO_LARGE               -13
#define SHTTP_RES_ERROR_STATUS_CODE                 -14
#define SHTTP_RES_PRINTF                            -15
#define SHTTP_RES_BODY_NOT_COMPLETE                 -16
#define SHTTP_RES_RESPONSE_ALREADY_FAILED           -17
#define SHTTP_RES_RESPONSE_BODY_IS_EMPTY            -18
#define SHTTP_RES_RESPONSE_ALREADY_END              -19
#define SHTTP_RES_INVALID_PARAMETERS                -20
/**@}*/

#define SHTTP_DELIMITER_STR ": "
#define SHTTP_DELIMITER_INT ((uint16_t)0x3a20)
#define SHTTP_DELIMITER_LEN 2
#define SHTTP_CRLF_STR      "\r\n"
#define SHTTP_CRLF_INT      ((uint16_t)0x0d0a)
#define SHTTP_CRLF_LEN      2


/** @name http server methods
 */
/**@{*/
DLL_VARIABLE shttp_t * shttp_create(shttp_settings_t* settings);

/**
* Listen announces on the local network address laddr.  The network string
* net must be a stream-oriented network: "tcp", "tcp4", "tcp6", "ipc".
* For TCP networks, addresses have the form host:port. If host is a literal
* IPv6 address, it must be enclosed in square brackets.
*/
DLL_VARIABLE shttp_res shttp_listen_at(shttp_t* http, const char *network, char *listen_addr, short port);
DLL_VARIABLE shttp_res shttp_run(shttp_t *server);
DLL_VARIABLE shttp_res shttp_shutdown(shttp_t *server);
DLL_VARIABLE void shttp_free(shttp_t *server);
/**@}*/



/** @name http connection methods
*/
/**@{*/

DLL_VARIABLE cstring_t* shttp_connection_strerr(shttp_connection_t *ctx);

DLL_VARIABLE shttp_res shttp_connection_call_after_disconnected(shttp_connection_t *ctx,
    shttp_write_cb cb, void *cb_data);

static inline void *shttp_connection_mem_malloc(shttp_connection_t *conn, size_t size) {
  return spool_malloc(conn->pool, size);
}

static inline void *shttp_connection_mem_calloc(shttp_connection_t *conn, size_t size) {
  void *p = spool_malloc(conn->pool, size);
  if(nil != p) {
    memset(p, 0, size);
  }
  return p;
}

static inline boolean shttp_connection_mem_try_realloc(shttp_connection_t *conn, void* p, size_t size) {
  return (nil != spool_try_realloc(conn->pool, p, size));
}

static inline void *shttp_connection_mem_realloc(shttp_connection_t *conn, void* p, size_t size) {
  return spool_realloc(conn->pool, p, size);
}

static inline void shttp_connection_mem_free(shttp_connection_t *conn, void *p) {
  spool_free(conn->pool, p);
}


DLL_VARIABLE void shttp_connection_c_free (shttp_connection_t *conn, void *data);
DLL_VARIABLE void shttp_connection_pool_free (shttp_connection_t *conn, void *data);
/**@}*/

/** @name http response methods
 */
/**@{*/
DLL_VARIABLE shttp_res shttp_response_call_after_writed(shttp_connection_t *ctx,
    shttp_write_cb cb, void *cb_data);

DLL_VARIABLE shttp_res shttp_response_call_after_completed(shttp_connection_t *ctx,
    shttp_write_cb cb, void *cb_data);

DLL_VARIABLE shttp_res shttp_response_start(shttp_connection_t *conn,
    uint16_t status,
    const char *content_type,
    size_t content_type_len);
DLL_VARIABLE shttp_res shttp_response_set_chuncked(shttp_connection_t *conn);
DLL_VARIABLE shttp_res shttp_response_write_header(shttp_connection_t *conn,
    const char *key,
    size_t     key_len,
    const char *value,
    size_t     value_len);
DLL_VARIABLE shttp_res shttp_response_write_header_format(shttp_connection_t *conn,
    const char *key,
    size_t     key_len,
    const char *fmt,
    ...);
DLL_VARIABLE shttp_res shttp_response_write(shttp_connection_t *conn,
    const char *data,
    size_t length,
    shttp_write_cb cb,
    void *cb_data);
DLL_VARIABLE shttp_res shttp_response_end(shttp_connection_t *conn);


static inline void *shttp_response_mem_malloc(shttp_connection_t *conn, size_t size) {
  return spool_malloc(conn->pool, size);
}

static inline void *shttp_response_mem_calloc(shttp_connection_t *conn, size_t size) {
  void *p = spool_malloc(conn->pool, size);
  if(nil != p) {
    memset(p, 0, size);
  }
  return p;
}

static inline boolean shttp_response_mem_try_realloc(shttp_connection_t *conn, void* p, size_t size) {
  return (nil != spool_try_realloc(conn->pool, p, size));
}

static inline void *shttp_response_mem_realloc(shttp_connection_t *conn, void* p, size_t size) {
  return spool_realloc(conn->pool, p, size);
}

static inline void shttp_response_mem_free(shttp_connection_t *conn, void *p) {
  spool_free(conn->pool, p);
}

DLL_VARIABLE void shttp_response_c_free (shttp_connection_t *conn, void *data);
DLL_VARIABLE void shttp_response_pool_free (shttp_connection_t *conn, void *data);
/**@}*/


/** @name http utility methods
 */
/**@{*/
extern cstring_t HTTP_CONTENT_HTML;
extern cstring_t HTTP_CONTENT_TEXT;

DLL_VARIABLE cstring_t* shttp_status_code_text(int status);

DLL_VARIABLE int shttp_parse_query_string(const char * query, size_t len, shttp_query_cb cb, void* ctx);

DLL_VARIABLE int shttp_notfound_handler(shttp_connection_t* conn);

/**
  A callback function used to intercept Libevent's log messages.

  @see event_set_log_callback
 */
typedef void (*shttp_log_cb_t)(void* ctx, int severity, const char *fmt, va_list ap);

/**
  Redirect shttp's log messages.

  @param cb a function taking two arguments: an integer severity between
     SHTTP_LOG_DEBUG and SHTTP_LOG_ERR, and a string.  If cb is NULL,
   then the default log is used.

  NOTE: The function you provide *must not* call any other shttp
  functionality.  Doing so can produce undefined behavior.
  */
DLL_VARIABLE void shttp_set_log_callback(shttp_log_cb_t cb, void* ctx);
/**@}*/

#ifdef __cplusplus
};
#endif

#endif //_SHTTP_H_
