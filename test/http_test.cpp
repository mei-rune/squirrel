#include "squirrel_config.h"
#include "http_test.h"


const char *simple_request= GET_REQUEST "\r\n";
const char *pipeline_requests= GET_REQUEST "\r\n" GET_REQUEST "\r\n";
const char *HELLO_WORLD_RESPONSE = "HTTP/1.1 200 OK\r\nabc: abc\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nContent-Length: 40\r\n\r\n<html><body>HELLO WORLD!!!</body></html>";
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


TEST(http, large_headers) {
  // large_headers will realloc memory while reading headers, and relocate buffers
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, GET_REQUEST, strlen(GET_REQUEST)));
  for(s =0; s < 200; s ++) {
    send_n(sock, "h234567890: 345678\r\n", 20);
  }
  send_n(sock, "\r\n", 2);

  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, strlen(HELLO_WORLD_RESPONSE));
  ASSERT_EQ( 0, strcmp(buf, HELLO_WORLD_RESPONSE));
  closesocket(sock);

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
  ASSERT_EQ( 0, strncmp(buf + strlen(HELLO_WORLD_RESPONSE), HELLO_WORLD_RESPONSE, strlen(HELLO_WORLD_RESPONSE)));
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
  ASSERT_EQ(true, send_n(sock, "1234567890" GET_REQUEST "123\r\n\r\n", strlen(GET_REQUEST) + 17));
  s = max_recv(sock, buf, 2048, RECV_TIMEOUT);
  ASSERT_EQ( s, 0);
  closesocket(sock);
  log_buf.str[log_buf.len] = 0;
  ASSERT_STREQ("parse error:HPE_INVALID_METHOD", log_buf.str);

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
  ASSERT_EQ( 0, strncmp(buf + strlen(HELLO_WORLD_RESPONSE), HELLO_WORLD_RESPONSE, strlen(HELLO_WORLD_RESPONSE)));
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


TEST(http, end_not_call_with_not_thunked) {
  WEB_DECL();
  uv_os_sock_t     sock;
  char             buf[2048];
  size_t           s;

  WEB_INIT();
  settings.callbacks.on_message_complete = &on_message_complete_not_thunked;
  WEB_START();

  sock = connect_tcp("127.0.0.1", TEST_PORT);
  ASSERT_EQ(true, send_n(sock, simple_request, strlen(simple_request)));
  s = max_recv(sock, buf, 2048, 2);
  ASSERT_EQ( s, strlen(BODY_NOT_COMPLETE));
  ASSERT_EQ(0, strcmp(buf, BODY_NOT_COMPLETE));
  closesocket(sock);

  WEB_STOP();
}