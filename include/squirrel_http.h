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

#define SHTTPE_MAX_HEADER_SIZ  256
  /**
   * max headers size, default is SHTTP_HEADER_SIZE_MAX
   */
  size_t max_headers_size;


#define SHTTP_MAC_BODY_SIZE    1024*1024*1024
  /**
   * max body bytes, default is SHTTP_MAC_BODY_SIZE
   */
  uint64_t max_body_size;

#define SHTTP_RW_BUFFER_SIZE   2*1024
  /**
   * the read buffer size per connection, default is SHTTP_RW_BUFFER_SIZE
   */
  size_t read_buffer_size;

  /**
   * the write buffer size per connection, default is SHTTP_RW_BUFFER_SIZE
   */
  size_t write_buffer_size;

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

  TAILQ_ENTRY(shttp_kv_s) next;
};

TAILQ_HEAD(shttp_kvs_s, shttp_kv_s);


typedef struct shttp_kv_array_s {
  size_t      capacity;
  size_t      length;
  shttp_kv_t  array;
} shttp_kv_array_t;

/**
 * @brief a structure containing all information for a http request.
 */
struct shttp_request_s {
  uint16_t           version;
  uint32_t           method;
  shttp_uri_t        uri;
  shttp_kv_array_t   headers;
};

/**
 * @brief a structure containing all information for a http request.
 */
struct shttp_response_s {
  uint16_t           status_code;
  cstring_t          content_type;
  boolean            connection;
  shttp_kv_array_t   headers;
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
typedef intptr_t                shttp_res;
#define SHTTP_RES_OK        0 /**< request completed ok */
#define SHTTP_RES_MEMORY    -1  /**< request does not have content */
#define SHTTP_RES_UV        -2  /**< request does not have content */
#define SHTTP_RES_NOTIMPLEMENT  -3  /**< the server is not available */

DLL_VARIABLE shttp_t * shttp_create(shttp_settings_t* settings);

/**
* Listen announces on the local network address laddr.  The network string
* net must be a stream-oriented network: "tcp", "tcp4", "tcp6", "ipc".
* For TCP networks, addresses have the form host:port. If host is a literal
* IPv6 address, it must be enclosed in square brackets.
*/
DLL_VARIABLE shttp_res shttp_listen_at(shttp_t* http, char *listen_addr, short port);
DLL_VARIABLE shttp_res shttp_run(shttp_t *server);
DLL_VARIABLE shttp_res shttp_shutdown(shttp_t *server);

// Shortcut functions to write a reponse
//shttp_res http_send_response_string(shttp_connection_t *conn, int status,
//        const char *extra_headers, const char *content_type,
//        const char *content, const uint32_t content_length,
//        shttp_write_cb cb, void *cb_data);
//
//shttp_res http_send_response_buffers(shttp_connection_t *conn, int status,
//        const char *extra_headers, const char *content_type,
//        cstring_t *content_buffers, int content_buffers_count,
//        shttp_write_cb content_cb, void *content_cb_data);

// Chuncked sending
shttp_res shttp_response_start(shttp_connection_t *conn, uint16_t status, cstring_t *content_type);
shttp_res shttp_response_set_chuncked(shttp_connection_t *conn);
shttp_res shttp_response_set_header(shttp_connection_t *conn,
                                    const char *key,
                                    size_t     key_len,
                                    const char *value,
                                    size_t     value_len,
                                    int        free_flag);
shttp_res shttp_response_write(shttp_connection_t *conn,
                               const char *data,
                               int length,
                               shttp_write_cb cb,
                               void *cb_data);
shttp_res shttp_response_end(shttp_connection_t *conn);



//DLL_VARIABLE http_server_t* http_init_from_config(char* configuration_filename);
//DLL_VARIABLE http_server_t* http_init_with_config(configuration* config);
//DLL_VARIABLE int http_http_open(http_server_t* srv);
//DLL_VARIABLE void http_http_add_route(char* route, http_request_callback callback, void* user_data);
//DLL_VARIABLE char* http_get_header(http_request* request, char* key);
//
//DLL_VARIABLE const char* http_status_code_text(int status);
//
//DLL_VARIABLE void http_free_response(shttp_response_t* response);
//DLL_VARIABLE void http_response_set_version(shttp_response_t* response, unsigned short major, unsigned short minor);
//DLL_VARIABLE void http_response_set_status_code(shttp_response_t* response, string_t* status_code);
//DLL_VARIABLE void http_response_set_header(shttp_response_t* response, string_t* name, string_t* value);
//DLL_VARIABLE void http_response_set_body(shttp_response_t* response, string_t* body);
//DLL_VARIABLE void http_response_send(shttp_response_t* response, void* user_data, http_response_complete_callback callback);
//



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
