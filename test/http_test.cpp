#include "squirrel_config.h"
#include "http_test.h"


const char *simple_request= GET_REQUEST "\r\n";
const char *pipeline_requests= GET_REQUEST "\r\n" GET_REQUEST "\r\n";
const char *HELLO_WORLD_RESPONSE = "HTTP/1.1 200 OK\r\n"
                                   "abc: abc\r\n"
                                   "Connection: keep-alive\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Content-Length: 40\r\n"
                                   "\r\n"
                                   "<html><body>HELLO WORLD!!!</body></html>";
cstring_t HTTP_CONTENT_TEXT_HTML = { 9, "text/html"};

TEST(http, create_and_free) {
  WEB_DECL();
  WEB_INIT();
  WEB_START();
  WEB_STOP();
}

TEST(http, simple) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));
  closesocket(sock);

  WEB_STOP();
}

TEST(http, async_simple) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  settings.callbacks.on_message_complete = &on_message_complete_async;
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));
  closesocket(sock);

  WEB_STOP();
}


//TEST(http, async_flush) {
//  WEB_DECL();
//  uv_os_sock_t     sock;
//  char             buf[2048];
//  size_t           s;
//
//  WEB_INIT();
//  settings.callbacks.on_message_complete = &on_message_complete_async_flush;
//  WEB_START();
//
//  sock = connect_tcp("127.0.0.1", TEST_PORT);
//  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
//  s = max_recv(sock, buf, 2048, 10 * RECV_TIMEOUT);
//  ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
//  ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));
//  closesocket(sock);
//
//  WEB_STOP();
//}

#define EMPTY_REPONSE "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n"
TEST(http, empty_message) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  settings.callbacks.on_message_complete = &on_message_complete_with_empty;
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(EMPTY_REPONSE));
  ASSERT_EQ( 0, strcmp(buf, EMPTY_REPONSE));
  closesocket(sock);

  WEB_STOP();
}

TEST(http, async_empty_message) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  settings.callbacks.on_message_complete = &on_message_complete_with_empty_async;
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(EMPTY_REPONSE));
  ASSERT_EQ( 0, strcmp(buf, EMPTY_REPONSE));
  closesocket(sock);

  WEB_STOP();
}


TEST(http, large_headers) {
  // large_headers will realloc memory while reading headers, and relocate buffers
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;
  int              i;

  WEB_INIT();
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  for(i =0; i < 3; i ++) {
    ASSERT_EQ(true, send_n(sock, GET_REQUEST, strlen(GET_REQUEST)));
    for(s =0; s < 200; s ++) {
      send_n(sock, "h234567890: 345678\r\n", 20);
    }
    send_n(sock, "\r\n", 2);

    s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
    ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
    ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));
  }
  closesocket(sock);

  WEB_STOP();
}

TEST(http, headers_count_to_large) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  shttp_set_log_callback(&on_log, &log_buf);
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, GET_REQUEST, strlen(GET_REQUEST)));
  for(s =0; s < 250; s ++) {
    send_n(sock, "h: 3\r\n", 6);
  }
  send_n(sock, "\r\n", 2);

  s = max_recv(sock, buf, 2048, 200 * RECV_TIMEOUT);
  ASSERT_EQ( s, 0);
  closesocket(sock);

  log_buf.str[log_buf.len] = 0;
  ASSERT_STREQ("parse error: header length too large.", log_buf.str);

  WEB_STOP();
}

TEST(http, pipeline_request) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, pipeline_requests, strlen(pipeline_requests)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( 0, strncmp(buf, HELLO_WORLD_RESPONSE, strlen(HELLO_WORLD_RESPONSE)));
  ASSERT_EQ( 0, strncmp(buf + strlen(HELLO_WORLD_RESPONSE),
                        HELLO_WORLD_RESPONSE, strlen(HELLO_WORLD_RESPONSE)));
  closesocket(sock);

  WEB_STOP();
}

TEST(http, async_pipeline_request) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  settings.callbacks.on_message_complete = &on_message_complete_with_async_pipeline_request;
  WEB_START();
  
  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, pipeline_requests, strlen(pipeline_requests)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( 0, strncmp(buf, HELLO_WORLD_RESPONSE, strlen(HELLO_WORLD_RESPONSE)));
  ASSERT_EQ( 0, strncmp(buf + strlen(HELLO_WORLD_RESPONSE),
                        HELLO_WORLD_RESPONSE, strlen(HELLO_WORLD_RESPONSE)));
  closesocket(sock);

  WEB_STOP();
}


TEST(http, send_error_request) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  shttp_set_log_callback(&on_log, &log_buf);
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock,
                         "1234567890" GET_REQUEST "123\r\n\r\n",
                         strlen(GET_REQUEST) + 17));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, 0);
  closesocket(sock);
  log_buf.str[log_buf.len] = 0;
  ASSERT_STREQ("parse error:HPE_INVALID_METHOD", log_buf.str);

  WEB_STOP();
}

TEST(http, connections_overflow) {
  WEB_DECL();
  uv_os_sock_t     sock[2];
  uv_os_sock_t     overflow;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  settings.max_connections_size = 2;
  //shttp_set_log_callback(&on_log, &log_buf);
  WEB_START();

  sock[0] = connect_tcp("127.0.0.1", TEST_PORT);
  sock[1] = connect_tcp("127.0.0.1", TEST_PORT);

  overflow = connect_tcp("127.0.0.1", TEST_PORT);

  s = max_recv(overflow, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, 0);
  closesocket(sock[0]);
  closesocket(sock[1]);
  closesocket(overflow);

  WEB_STOP();
}

TEST(http, pipeline_request_while_two_read) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, pipeline_requests, strlen(pipeline_requests)-12));
  ASSERT_EQ(true, send_n(sock, pipeline_requests + (strlen(pipeline_requests)-12), 12));

  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( 0, strncmp(buf, HELLO_WORLD_RESPONSE, strlen(HELLO_WORLD_RESPONSE)));
  ASSERT_EQ( 0, strncmp(buf + strlen(HELLO_WORLD_RESPONSE),
                        HELLO_WORLD_RESPONSE, strlen(HELLO_WORLD_RESPONSE)));
  closesocket(sock);

  WEB_STOP();
}

TEST(http, async_pipeline_request_while_two_read) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  settings.callbacks.on_message_complete = &on_message_complete_with_async_pipeline_request;
  WEB_START();
  
  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, pipeline_requests, strlen(pipeline_requests)-12));
  ASSERT_EQ(true, send_n(sock, pipeline_requests + (strlen(pipeline_requests)-12), 12));

  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( 0, strncmp(buf, HELLO_WORLD_RESPONSE, strlen(HELLO_WORLD_RESPONSE)));
  ASSERT_EQ( 0, strncmp(buf + strlen(HELLO_WORLD_RESPONSE),
                        HELLO_WORLD_RESPONSE, strlen(HELLO_WORLD_RESPONSE)));
  closesocket(sock);

  WEB_STOP();
}


TEST(http, reuse_connect) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;


  WEB_INIT();
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));


  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));

  closesocket(sock);


  WEB_STOP();
}

TEST(http, async_reuse_connect) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  settings.callbacks.on_message_complete = &on_message_complete_muti_write_async;
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));


  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));

  closesocket(sock);

  WEB_STOP();
}

TEST(http, muti_write) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));
  closesocket(sock);

  WEB_STOP();
}

TEST(http, async_muti_write) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  settings.callbacks.on_message_complete = &on_message_complete_muti_write_async;
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));
  closesocket(sock);

  WEB_STOP();
}

#ifdef SHTTP_THREAD_SAFE
TEST(http, async_check_thread_id) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  shttp_assert_ctx = &assert_buf;
  shttp_assert_cb = &on_assert;
  settings.callbacks.on_message_complete = &on_message_complete_async_check_thread_self;
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  //ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  //ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));
  closesocket(sock);

  WEB_STOP();
  
  assert_buf.str[assert_buf.len] = 0;
  ASSERT_STREQ("conn_outgoing(conn).thread_id == uv_thread_self()", assert_buf.str);
}


TEST(http, async_check_writing) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  shttp_set_log_callback(&on_log, &log_buf);
  shttp_assert_ctx = &assert_buf;
  shttp_assert_cb = &on_assert;
  settings.callbacks.on_message_complete = &on_message_complete_async_check_is_writing;
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, 20*RECV_TIMEOUT);
  //ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  //ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));
  closesocket(sock);

  WEB_STOP();
  
  assert_buf.str[assert_buf.len] = 0;
  ASSERT_STREQ("0 == shttp_atomic_read32(&conn_outgoing(conn).is_writing)", assert_buf.str);
}
#endif

void on_message_send (shttp_connection_t* conn, void *act) {
  usr_context_t *ctx = (usr_context_t*)act;

  switch(ctx->status) {
  case 0:
    shttp_response_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
    shttp_response_write_header(conn, "abc", 3, "abc", 3);
    shttp_response_write(conn, "abc", 3, &on_message_send, act);
    break;
  case 1:
    shttp_response_write(conn, "abc", 3, &on_message_send, act);
    break;
  case 2:
    shttp_response_end(conn);
    break;
  }
}

int on_message_complete_not_thunked(shttp_connection_t* conn) {
  usr_context_t *ctx = (usr_context_t*)conn->ctx;
  ctx->status = 0;
  on_message_send(conn, ctx);
  return 0;
}
void on_message_send_async (shttp_connection_t* conn, void *act) {
  usr_context_t *ctx = (usr_context_t*)act;

  shttp_response_async_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_async_write_header(conn, "abc", 3, "abc", 3);
  shttp_response_async_write(conn, "abc", 3, nil, nil);
  shttp_response_async_flush(conn, &on_message_send, act);
  shttp_response_async_end(conn, nil, nil);
}

static inline void async_not_thunked(void *act) {
  shttp_connection_t* conn = (shttp_connection_t*)act;
  usr_context_t *ctx = (usr_context_t*)conn->ctx;
  ctx->status = 0;
  on_message_send_async(conn, ctx);
}

int on_message_complete_async_not_thunked(shttp_connection_t* conn) {
  uv_thread_t tid;
  shttp_response_set_async(conn);  
  uv_thread_create(&tid, &async_not_thunked, conn);
  return 0;
}


TEST(http, end_not_call_with_not_thunked) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  shttp_set_log_callback(&on_log, &log_buf);
  settings.callbacks.on_message_complete = &on_message_complete_not_thunked;
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(BODY_NOT_COMPLETE));
  ASSERT_EQ(0, strcmp(buf, BODY_NOT_COMPLETE));
  closesocket(sock);

  log_buf.str[log_buf.len] = 0;
  ASSERT_STREQ("callback: chunked must is true while body is not completed.", log_buf.str);

  WEB_STOP();
}

TEST(http, async_end_not_call_with_not_thunked) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  shttp_set_log_callback(&on_log, &log_buf);
  settings.callbacks.on_message_complete = &on_message_complete_async_not_thunked;
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  //ASSERT_EQ(s, strlen(BODY_NOT_COMPLETE));
  //ASSERT_EQ(0, strcmp(buf, BODY_NOT_COMPLETE));
  closesocket(sock);

  log_buf.str[log_buf.len] = 0;
  ASSERT_STREQ("callback: chunked must is true while body is not completed.", log_buf.str);

  WEB_STOP();
}

int on_message_begin_with_reply(shttp_connection_t* conn) {
  return on_message_complete(conn);
}

int on_message_complete_empty(shttp_connection_t* conn) {
  return 0;
}

TEST(http, reply_in_begin) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  settings.callbacks.on_message_begin = &on_message_begin_with_reply;
  settings.callbacks.on_message_complete = &on_message_complete_empty;
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));
  closesocket(sock);

  WEB_STOP();
}