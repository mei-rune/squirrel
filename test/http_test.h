
#ifndef _http_test_h_
#define _http_test_h_ 1

#include "squirrel_config.h"

#include <stdlib.h>
#include "uv.h"
#include "internal.h"
#include "squirrel_test.h"
#include "squirrel_http.h"

#define RECV_TIMEOUT 200
#define TEST_PORT 8456

static void startup(void) {
#ifdef _WIN32
  struct WSAData wsa_data;
  int r = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  assert(r == 0);
#endif
}

enum select_mode {
  select_read  = 1,
  select_write = 2,
  select_error = 4
};

static inline bool socket_set_option (uv_os_sock_t sock,
                        int level,
                        int option,
                        void *optval,
                        int optlen) {
  return ( SOCKET_ERROR != setsockopt (sock, level,
                                       option, (char *) optval, optlen) );
}

static inline bool socket_get_option (uv_os_sock_t sock,
                        int level,
                        int option,
                        void *optval,
                        int *optlen) {
  return ( SOCKET_ERROR != getsockopt (sock, level,
                                       option, (char *) optval, optlen) );
}

static inline bool socket_enable (uv_os_sock_t sock, int value) {
  u_long nonblock = 1;
  return ( 0 ==::ioctlsocket (sock,
                              value,
                              &nonblock));
}

static inline bool socket_disable (uv_os_sock_t sock, int value) {
  u_long nonblock = 0;
  return ( 0 == ioctlsocket (sock,
                             value,
                             &nonblock));
}

static inline bool socket_poll(uv_os_sock_t sock, const TIMEVAL& time_val, int mode) {
  fd_set socket_set;
  FD_ZERO( &socket_set );
  FD_SET(sock, &socket_set );

  return ( 1 == ::select( 0, (mode & select_read)?&socket_set:NULL
                          , (mode & select_write)?&socket_set:NULL
                          , (mode & select_error)?&socket_set:NULL
                          , &time_val ) );
}

static inline bool socket_isReadable(uv_os_sock_t sock) {
  TIMEVAL time_val;
  time_val.tv_sec = 0;
  time_val.tv_usec = 0;
  return socket_poll(sock, time_val, select_read );
}

static inline bool socket_isWritable(uv_os_sock_t sock) {
  TIMEVAL time_val;
  time_val.tv_sec = 0;
  time_val.tv_usec = 0;
  return socket_poll(sock, time_val, select_write );
}

static inline void socket_setBlocking(uv_os_sock_t sock, bool val) {
  if( val )
    socket_enable(sock, FIONBIO);
  else
    socket_disable(sock, FIONBIO);
}

static inline bool send_n(uv_os_sock_t sock, const char* buf, size_t length) {
  do {
#pragma warning(disable: 4267)
    int n = ::send(sock, buf, length, 0 );
#pragma warning(default: 4267)
    if( 0 >= n)
      return false;

    length -= n;
    buf += n;
  } while( 0 < length );

  return true;
}

static inline bool recv_n(uv_os_sock_t sock, char* buf, size_t length) {
  do {
#pragma warning(disable: 4267)
    int n = ::recv(sock, buf, length, 0 );
#pragma warning(default: 4267)

    if( 0 >= n)
      return false;

    length -= n;
    buf += n;
  } while( 0 < length );

  return true;
}
//
//static inline bool sendv_n(uv_os_sock_t sock, uv_buf_t* wsaBuf, size_t size) {
//  uv_buf_t* p = wsaBuf;
//
//  do {
//    DWORD numberOfBytesSent =0;
//#pragma warning(disable: 4267)
//    if( SOCKET_ERROR == ::WSASend(sock, p, size, &numberOfBytesSent,0,0 , 0 ) )
//#pragma warning(default: 4267)
//      return false;
//
//    do {
//      if( numberOfBytesSent < p->len) {
//        p->len -= numberOfBytesSent;
//        p->base = p->base + numberOfBytesSent;
//        break;
//      }
//      numberOfBytesSent -= p->len;
//      ++ p;
//      -- size;
//    } while( 0 < numberOfBytesSent );
//  } while( 0 < size );
//
//  return true;
//}
//
//static inline bool recvv_n(uv_os_sock_t sock, uv_buf_t* wsaBuf, size_t size) {
//  uv_buf_t* p = wsaBuf;
//
//  do {
//    DWORD numberOfBytesRecvd =0;
//#pragma warning(disable: 4267)
//    if( SOCKET_ERROR == ::WSARecv(sock, p, size, &numberOfBytesRecvd,0,0 , 0 ) )
//#pragma warning(default: 4267)
//      return false;
//
//    do {
//      if( numberOfBytesRecvd < p->len) {
//        p->len -= numberOfBytesRecvd;
//        p->base = p->base + numberOfBytesRecvd;
//        break;
//      }
//      numberOfBytesRecvd -= p->len;
//      ++ p;
//      -- size;
//    } while( 0 < numberOfBytesRecvd );
//  } while( 0 < size );
//
//  return true;
//}


static inline size_t max_recv(uv_os_sock_t sock, char* simple_request, size_t simple_request_len, int timeout) {
  size_t    recv_size = 0;
  int       n;
  TIMEVAL   time_val;
  uint64_t  usec;

  time_val.tv_sec = 0;
  usec = ((uint64_t)timeout) * 1000;
  while(1000000 <= usec) {
    time_val.tv_sec ++;
    usec -= 1000000;
  }
  time_val.tv_usec = (long)usec;

  while(socket_poll(sock, time_val, select_read )) {
    n = recv(sock, simple_request + recv_size, (int)(simple_request_len - recv_size), 0);
    if(0 >= n) {
      break;
    }
    recv_size += n;
    simple_request[recv_size] =0;
  }
  return recv_size;
}

static inline uv_os_sock_t create_tcp_socket(void) {
  uv_os_sock_t sock;

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
#ifdef _WIN32
  assert(sock != INVALID_SOCKET);
#else
  assert(sock >= 0);
#endif

#ifndef _WIN32
  {
    /* Allow reuse of the port. */
    int yes = 1;
    int r = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    ASSERT(r == 0);
  }
#endif

  return sock;
}

#define GET_MINI_REQUEST "GET /abc.ico HTTP/1.1\r\n" \
                  "Host: 0.0.0.0=5000\r\n"           \
                  "Keep-Alive: 300\r\n"              \
                  "Connection: keep-alive\r\n"

#define GET_REQUEST "GET /abc.ico HTTP/1.1\r\n" \
                  "Host: 0.0.0.0=5000\r\n"          \
                  "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n" \
                  "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"                  \
                  "Accept-Language: en-us,en;q=0.5\r\n"                                                          \
                  "Accept-Encoding: gzip,deflate\r\n"                                                            \
                  "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"                                           \
                  "Keep-Alive: 300\r\n"                                                                          \
                  "Connection: keep-alive\r\n"

#define HEAD_ABC "0123456789"
#define HEAD_ABC_X_5   HEAD_ABC HEAD_ABC HEAD_ABC HEAD_ABC HEAD_ABC
#define HEAD_ABC_X_10  HEAD_ABC_X_5 HEAD_ABC_X_5
#define HEAD_ABC_X_20  HEAD_ABC_X_10 HEAD_ABC_X_10
#define HEAD_ABC_X_50  HEAD_ABC_X_10 HEAD_ABC_X_10 HEAD_ABC_X_10 HEAD_ABC_X_10 HEAD_ABC_X_10 HEAD_ABC_X_10
#define HEAD_ABC_X_100 HEAD_ABC_X_50 HEAD_ABC_X_50


#define HEAD_SIMPLE "H0123456789: 0123456789\r\n"
#define HEAD_SIMPLE_X_5   HEAD_SIMPLE HEAD_SIMPLE HEAD_SIMPLE HEAD_SIMPLE HEAD_SIMPLE
#define HEAD_SIMPLE_X_10  HEAD_SIMPLE_X_5 HEAD_SIMPLE_X_5
#define HEAD_SIMPLE_X_20  HEAD_SIMPLE_X_10 HEAD_SIMPLE_X_10
#define HEAD_SIMPLE_X_50  HEAD_SIMPLE_X_10 HEAD_SIMPLE_X_10 HEAD_SIMPLE_X_10 HEAD_SIMPLE_X_10 HEAD_SIMPLE_X_10 HEAD_SIMPLE_X_10
#define HEAD_SIMPLE_X_100 HEAD_SIMPLE_X_50 HEAD_SIMPLE_X_50


#define HEAD_MINI_SIMPLE "H: V\r\n"
#define HEAD_MINI_SIMPLE_X_5   HEAD_MINI_SIMPLE HEAD_MINI_SIMPLE HEAD_MINI_SIMPLE HEAD_MINI_SIMPLE HEAD_MINI_SIMPLE
#define HEAD_MINI_SIMPLE_X_10  HEAD_MINI_SIMPLE_X_5 HEAD_MINI_SIMPLE_X_5
#define HEAD_MINI_SIMPLE_X_20  HEAD_MINI_SIMPLE_X_10 HEAD_MINI_SIMPLE_X_10
#define HEAD_MINI_SIMPLE_X_50  HEAD_MINI_SIMPLE_X_10 HEAD_MINI_SIMPLE_X_10 HEAD_MINI_SIMPLE_X_10 HEAD_MINI_SIMPLE_X_10 HEAD_MINI_SIMPLE_X_10 HEAD_MINI_SIMPLE_X_10
#define HEAD_MINI_SIMPLE_X_100 HEAD_MINI_SIMPLE_X_50 HEAD_MINI_SIMPLE_X_50
#define HEAD_MINI_SIMPLE_X_200 HEAD_MINI_SIMPLE_X_100 HEAD_MINI_SIMPLE_X_100

#define HELLO_WORLD "<html><body>HELLO WORLD!!!</body></html>"
#define HELLO_WORLD_X_5   HELLO_WORLD HELLO_WORLD HELLO_WORLD HELLO_WORLD HELLO_WORLD
#define HELLO_WORLD_X_10  HELLO_WORLD_X_5 HELLO_WORLD_X_5
#define HELLO_WORLD_X_50  HELLO_WORLD_X_10 HELLO_WORLD_X_10 HELLO_WORLD_X_10 HELLO_WORLD_X_10 HELLO_WORLD_X_10 HELLO_WORLD_X_10
#define HELLO_WORLD_X_100 HELLO_WORLD_X_50 HELLO_WORLD_X_50

extern const char *simple_request;
extern const char *pipeline_requests;
extern const char *HELLO_WORLD_RESPONSE;
extern cstring_t  HTTP_CONTENT_TEXT_HTML;

typedef struct usr_context_s {
  int                 status;
  void                *data1;
  void                *data2;
} usr_context_t;


static inline void on_write_frist(shttp_connection_t* conn, void* act) {
  usr_context_t *ctx = (usr_context_t*)conn->connection_context;
  ctx->data1 = act;
}

static inline void on_write_second(shttp_connection_t* conn, void* act) {
  usr_context_t *ctx = (usr_context_t*)conn->connection_context;
  ctx->data2 = act;
}

void on_assert(void* ctx, const char* msg, const char* file, int line) {
  sbuffer_t *s;
  s = (sbuffer_t*)ctx;
  s->len = strlen(msg);
  s->len = (s->len >= s->capacity)? s->capacity : s->len;
  strncpy(s->str, msg, s->len);
}

static inline void on_log(void* ctx, int severity, const char *fmt, va_list ap) {
  sbuffer_t *s;
  s = (sbuffer_t*)ctx;
  
  s->len = vsnprintf(s->str, s->capacity, fmt, ap);
}

static inline int  on_message_begin (shttp_connection_t* conn) {
  return 0;
}

static inline int  on_body (shttp_connection_t* conn, const char *at, size_t length) {
  return 0;
}

static inline int  on_message_complete (shttp_connection_t* conn) {
  usr_context_t *ctx = (usr_context_t*)conn->connection_context;
  ctx->status = 0;
  shttp_response_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_write_header(conn, "abc", 3, "abc", 3);
  shttp_response_write(conn, HELLO_WORLD, strlen(HELLO_WORLD), &on_write_frist, nil);
  shttp_response_end(conn);
  return 0;
}


static inline int  on_message_complete_format (shttp_connection_t* conn) {
  usr_context_t *ctx = (usr_context_t*)conn->connection_context;
  ctx->status = 0;
  shttp_response_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_format_header(conn, "abc", 3, "%s", "abc");
  shttp_response_write_copy(conn, HELLO_WORLD, strlen(HELLO_WORLD) -10);
  shttp_response_format(conn, "%s", HELLO_WORLD + (strlen(HELLO_WORLD) -10));
  shttp_response_end(conn);
  return 0;
}

static inline int  on_message_complete_format_realloc (shttp_connection_t* conn) {
  usr_context_t *ctx = (usr_context_t*)conn->connection_context;
  ctx->status = 0;
  shttp_response_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_format_header(conn, "abc", 3, "%s", HEAD_ABC_X_20 HEAD_ABC_X_10 HEAD_ABC_X_5);
  shttp_response_format(conn,  "%s", HELLO_WORLD);
  shttp_response_write_copy(conn, HELLO_WORLD, strlen(HELLO_WORLD));
  shttp_response_format(conn, "%s", HELLO_WORLD_X_50);
  shttp_response_end(conn);
  return 0;
}

static inline int  on_message_complete_with_empty (shttp_connection_t* conn) {
  shttp_response_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_end(conn);
  return 0;
}

static inline int  on_message_complete_muti_write(shttp_connection_t* conn) {
  usr_context_t *ctx = (usr_context_t*)conn->connection_context;
  ctx->status = 0;

  shttp_response_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_write_header(conn, "abc", 3, "abc", 3);
  shttp_response_write(conn, HELLO_WORLD, strlen(HELLO_WORLD) -3, on_write_frist,  nil);
  shttp_response_write(conn, HELLO_WORLD + (strlen(HELLO_WORLD) -3), 3, on_write_second, nil);
  assert(SHTTP_RES_OK == shttp_response_async_end(conn, nil, nil));
  return 0;
}

static inline void async_write_empty_response(void *act) {
  shttp_connection_t* conn = (shttp_connection_t*)act;
  shttp_response_async_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  assert(SHTTP_RES_OK == shttp_response_async_end(conn, nil, nil));
}

static inline void async_write_response(void *act) {
  shttp_connection_t* conn = (shttp_connection_t*)act;
  shttp_response_async_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_async_write_header(conn, "abc", 3, "abc", 3);
  shttp_response_async_write(conn, HELLO_WORLD, strlen(HELLO_WORLD), &on_write_frist, nil);
  assert(SHTTP_RES_OK == shttp_response_async_end(conn, nil, nil));
}

static inline int  on_message_complete_async (shttp_connection_t* conn) {
  uv_thread_t tid;
  shttp_response_set_async(conn);  
  uv_thread_create(&tid, &async_write_response, conn);
  return 0;
}

static inline int on_message_complete_with_empty_async(shttp_connection_t *conn) {
  uv_thread_t tid;
  shttp_response_set_async(conn);  
  uv_thread_create(&tid, &async_write_empty_response, conn);
  return 0;
}

static inline void muti_async_write_response(void *act) {
  shttp_connection_t* conn = (shttp_connection_t*)act;

  shttp_response_async_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_async_write_header(conn, "abc", 3, "abc", 3);
  shttp_response_async_write(conn, HELLO_WORLD, strlen(HELLO_WORLD) -3, on_write_frist,  nil);
  shttp_response_async_write(conn, HELLO_WORLD + (strlen(HELLO_WORLD) -3), 3, on_write_second, nil);
  assert(SHTTP_RES_OK == shttp_response_async_end(conn, nil, nil));
}

static inline void async_write_response_no_start(void *act) {
  shttp_connection_t* conn = (shttp_connection_t*)act;
  shttp_response_async_write_header(conn, "abc", 3, "abc", 3);
  shttp_response_async_write(conn, HELLO_WORLD, strlen(HELLO_WORLD), &on_write_frist, nil);
  assert(SHTTP_RES_THREAD_SAFE == shttp_response_async_end(conn, nil, nil));
}

static inline int on_message_complete_muti_write_async(shttp_connection_t* conn) {
  uv_thread_t tid;
  shttp_response_set_async(conn);  
  uv_thread_create(&tid, &muti_async_write_response, conn);
  return 0;
}

static inline int  on_message_complete_async_check_thread_self(shttp_connection_t* conn) {
  uv_thread_t tid;
  shttp_response_set_async(conn);
  shttp_response_async_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  uv_thread_create(&tid, &async_write_response_no_start, conn);
#ifdef _WIN32
  Sleep(500);
#else
  sleep(1);
#endif
  assert(SHTTP_RES_OK == shttp_response_async_end(conn, nil, nil));
  return 0;
}

static inline void  async_check_is_writing(void *act) {
  shttp_connection_t* conn = (shttp_connection_t*)act;
  shttp_response_set_async(conn);
  shttp_response_async_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_async_write(conn, HELLO_WORLD, strlen(HELLO_WORLD), on_write_frist,  nil);
  shttp_response_set_chuncked(conn);
  shttp_response_async_flush(conn, nil, nil);
  shttp_response_async_write(conn, HELLO_WORLD + (strlen(HELLO_WORLD) -3), 3, on_write_second, nil);
#ifdef _WIN32
  Sleep(500);
#else
  sleep(1);
#endif
  assert(SHTTP_RES_OK == shttp_response_async_end(conn, nil, nil));
}
static inline int  on_message_complete_async_check_is_writing(shttp_connection_t* conn) {
  uv_thread_t tid;
  shttp_response_set_async(conn);  
  uv_thread_create(&tid, &async_check_is_writing, conn);
  return 0;
}

static inline void async_write_async_flush(void *act) {
  shttp_connection_t* conn = (shttp_connection_t*)act;
  shttp_response_async_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_async_write_header(conn, "abc", 3, "abc", 3);
  assert(0 == shttp_response_async_flush(conn, nil, nil));
#ifdef _WIN32
  Sleep(400);
#else
  sleep(1);
#endif
  shttp_response_async_write(conn, HELLO_WORLD, strlen(HELLO_WORLD), &on_write_frist, nil);
  assert(SHTTP_RES_OK == shttp_response_async_end(conn, nil, nil));
}

static inline int on_message_complete_async_flush(shttp_connection_t *conn) {
  uv_thread_t tid;
  shttp_response_set_async(conn);  
  uv_thread_create(&tid, &async_write_async_flush, conn);
  return 0;
}

static inline void async_pipeline_request(void *act) {
  shttp_connection_t* conn = (shttp_connection_t*)act;
  shttp_response_async_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_async_write_header(conn, "abc", 3, "abc", 3);
  shttp_response_async_write(conn, HELLO_WORLD, strlen(HELLO_WORLD), &on_write_frist, nil);
  assert(SHTTP_RES_OK == shttp_response_async_end(conn, nil, nil));
}

static inline int on_message_complete_with_async_pipeline_request(shttp_connection_t *conn) {
  uv_thread_t tid;
  shttp_response_set_async(conn);  
  uv_thread_create(&tid, &async_pipeline_request, conn);
  return 0;
}



static inline void start_web(void *arg) {
  shttp_t  *srv = (shttp_t*)arg;
  assert(SHTTP_RES_OK == shttp_run(srv));
}

static inline uv_os_sock_t connect_tcp(const char* remote, int port) {
  struct sockaddr_in     addr;
  uv_os_sock_t           sock;

  sock = create_tcp_socket();
  assert(0 == uv_ip4_addr(remote, port, &addr));
  sock = create_tcp_socket();
  assert(0 == connect(sock, (sockaddr*) &addr, sizeof(struct sockaddr_in)));

  return sock;
}

//static inline void is_valid_response(const char* response, size_t response_len) {
//  http_parser_settings settings;
//  http_parser parser;
//
//  settings.on_url = test_on_url;
//  settings.on_header_field = test_on_header_field;
//
//  http_parser_init(parser, HTTP_REQUEST);
//  parser->data = my_socket;
//}

#define WEB_DECL()                                                 \
  shttp_settings_t settings;                                       \
  shttp_t          *srv;                                           \
  char             log_mem[1025];                                  \
  char             assert_mem[1025];                               \
  sbuffer_t        log_buf;                                        \
  sbuffer_t        assert_buf;                                     \
  uv_thread_t      tid

#define WEB_INIT()                                                 \
  sbuffer_init(log_buf, log_mem, 0, 1024);                         \
  sbuffer_init(assert_buf, assert_mem, 0, 1024);                   \
  shttp_set_log_callback(nil, nil);                                \
  shttp_assert_ctx = nil;                                          \
  shttp_assert_cb = nil;                                           \
  memset(&settings, 0, sizeof(shttp_settings_t));                  \
  settings.user_ctx_size = sizeof(usr_context_t);                  \
  settings.callbacks.on_message_begin = &on_message_begin;         \
  settings.callbacks.on_body = &on_body;                           \
  settings.callbacks.on_message_complete = &on_message_complete



#define WEB_START()                                                 \
  ASSERT_NE(nil, (srv = shttp_create(&settings)));                  \
  ASSERT_EQ(SHTTP_RES_OK, shttp_listen_at(srv,                      \
                         "tcp4", "0.0.0.0", TEST_PORT));            \
  uv_thread_create(&tid, &start_web, srv)



#define WEB_STOP()                                                 \
  ASSERT_EQ(SHTTP_RES_OK, shttp_shutdown(srv));                    \
  uv_thread_join(&tid);                                            \
  shttp_free(srv)


#endif //_http_test_h_