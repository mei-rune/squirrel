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
#define SHTTP_METHOD_MAP(XX)       \
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

typedef int  (*shttp_data_cb) (shttp_connection_t*, const char *at, size_t length);
typedef int  (*shttp_cb) (shttp_connection_t*);
typedef void (*shttp_write_cb) (void *data);


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
  shttp_cb        on_message_begin;
  shttp_data_cb   on_body;
  shttp_cb        on_message_complete;
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

#define SHTTPE_MAX_HEADER_SIZE  4*1024
  /**
   * max headers size, default is SHTTP_HEADER_SIZE_MAX
   */
  size_t max_headers_size;


#define SHTTP_MAC_BODY_SIZE    1024*1024*1024
  /**
   * max body bytes, default is SHTTP_MAC_BODY_SIZE
   */
  uint64_t max_body_size;

#define SHTTP_RW_BUFFER_SIZE   2*1024*1024
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

/**
 * @brief a structure containing all information for a http request.
 */
struct shttp_response_s {
  uint16_t           http_major;
  uint16_t           http_minor;
  uint64_t           status_code:16;
  uint64_t           close_connection:1;
  uint64_t           chunked:1;
  uint64_t           head_writed:1;
  uint64_t           reserved:13;

  cstring_t          content_type;
  uint64_t           content_length;
  shttp_header_t     headers;
};

struct shttp_connection_s {
  shttp_t           *http;
  shttp_request_t   request;
  shttp_response_t  response;
  void              *ctx;
  void              *internal;
};


struct shttp_write_cb_s {
  // Callback info to free buffers
  shttp_write_cb cb;
  void *data;
};

/* Response codes */
typedef intptr_t                      shttp_res;
#define SHTTP_RES_OK                  0
#define SHTTP_RES_ERROR               -1
#define SHTTP_RES_MEMORY              -2
#define SHTTP_RES_UV                  -3
#define SHTTP_RES_NOTIMPLEMENT        -4
#define SHTTP_RES_HEAD_WRITED         -5
#define SHTTP_RES_HEAD_TOO_LARGE      -6

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


DLL_VARIABLE shttp_res shttp_response_start(shttp_connection_t *conn, uint16_t status, cstring_t *content_type);
DLL_VARIABLE shttp_res shttp_response_set_chuncked(shttp_connection_t *conn);

#define SHTTP_HEAD_COPY             (0)
#define SHTTP_MEM_COPY              0
#define SHTTP_MEM_NOFREE            1
#define SHTTP_MEM_RESFREE           2
#define SHTTP_MEM_CFREE             3

#define SHTTP_HEAD_KEY_COPY         (SHTTP_MEM_COPY<<8)
#define SHTTP_HEAD_KEY_NOFREE       (SHTTP_MEM_NOFREE<<8)
#define SHTTP_HEAD_KEY_RESFREE      (SHTTP_MEM_RESFREE<<8)
#define SHTTP_HEAD_KEY_CFREE        (SHTTP_MEM_CFREE<<8)

#define SHTTP_HEAD_VAL_COPY         SHTTP_MEM_COPY
#define SHTTP_HEAD_VAL_NOFREE       SHTTP_MEM_NOFREE
#define SHTTP_HEAD_VAL_RESFREE      SHTTP_MEM_RESFREE
#define SHTTP_HEAD_VAL_CFREE        SHTTP_MEM_CFREE

DLL_VARIABLE shttp_res shttp_response_set_header(shttp_connection_t *conn,
                                    const char *key,
                                    size_t     key_len,
                                    const char *value,
                                    size_t     value_len,
                                    int        flag);
DLL_VARIABLE shttp_res shttp_response_write(shttp_connection_t *conn,
                               const char *data,
                               int length,
                               shttp_write_cb cb,
                               void *cb_data);
DLL_VARIABLE shttp_res shttp_response_end(shttp_connection_t *conn);

/**
  A callback function used to intercept Libevent's log messages.

  @see event_set_log_callback
 */
typedef void (*shttp_log_cb_t)(int severity, const char *fmt, va_list ap);

/**
  Redirect shttp's log messages.

  @param cb a function taking two arguments: an integer severity between
     SHTTP_LOG_DEBUG and SHTTP_LOG_ERR, and a string.  If cb is NULL,
   then the default log is used.

  NOTE: The function you provide *must not* call any other shttp
  functionality.  Doing so can produce undefined behavior.
  */
DLL_VARIABLE void shttp_set_log_callback(shttp_log_cb_t cb);

#ifdef __cplusplus
};
#endif

#endif //_SHTTP_H_
