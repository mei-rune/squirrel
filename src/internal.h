
#ifndef _HTTP_INTERNAL_H_
#define _HTTP_INTERNAL_H_

#include "squirrel_config.h"

#ifdef _MSC_VER
 #include <BaseTsd.h>
 #include "sys/queue.h"
#else
 #include <sys/queue.h>
#endif

#include "uv.h"
#include "http_parser.h"
#include "util.h"
#include "shttp.h"
#include "sstring.h"

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
    shttp_message_url     = 1,
    shttp_message_field   = 2,
    shttp_message_value   = 3,
    shttp_message_body    = 4,
    shttp_message_end     = 5
  }shttp_message_status_t;

  typedef struct shttp_parser_s {
	struct http_parser          inner;
	shttp_message_status_t      status;
	cstring_t                   stack[2];
	
    shttp_callbacks_t           *callbacks;
	shttp_connection_t          *conn;
  } shttp_parser_t;

  typedef struct shttp_buffer_s {
	  char*  base;
	  size_t capacity;
	  size_t start;
	  size_t end;
  } shttp_buffer_t;

  typedef struct shttp_connection_internal_s {
	shttp_connection_t      inner;
	uv_tcp_t                uv_handle;
	shttp_parser_t          parser;
	shttp_buffer_t          incoming;
	shttp_buffer_t          outgoing;

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

#define LOG_HTTP(at, len, fmt, ...) _http_log_data(at, len, fmt "\n", __VA_ARGS__);
#define LOG(level, ...) _shttp_log_printf(level, __VA_ARGS__)
#define TRACE(...) _shttp_log_printf(SHTTP_LOG_DEBUG, __VA_ARGS__)
#define WARN(...)  _shttp_log_printf(SHTTP_LOG_WARN, __VA_ARGS__)
#define ERR(...) _shttp_log_printf(SHTTP_LOG_ERR, __VA_ARGS__)
#define CRIT(...)  _shttp_log_printf(SHTTP_LOG_CRIT, __VA_ARGS__)

  // Log Functions
  void _http_log_data(const char* data, size_t len, const char* fmt, ...);
  extern void _shttp_log_printf(int severity, const char *fmt, ...);


  extern int shttp_win32_version;
  extern int shttp_pagesize;
  extern int shttp_ncpu;
  extern int shttp_cacheline_size;
  extern int shttp_pagesize_shift;
  extern void os_init();

#ifdef __cplusplus
}
#endif

#endif //_INTERNAL_H_
