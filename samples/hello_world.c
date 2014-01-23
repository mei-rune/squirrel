
#include "squirrel_http.h"
#include <string.h>

#define HELLO_WORLD "<html><body>HELLO WORLD!!!</body></html>"

static cstring_t HTTP_CONTENT_TEXT_HTML = { 9, "text/html"};
static cstring_t HTTP_HELLO_WORLD = { 9, HELLO_WORLD};

typedef struct usr_context_s {
  int                 status;
} usr_context_t;


void on_log(int severity, const char *fmt, va_list ap) {
  _vscprintf(fmt, ap);
}

int  on_message_begin (shttp_connection_t* conn) {
  return 0;
}

int  on_body (shttp_connection_t* conn, const char *at, size_t length) {
  return 0;
}

void on_message_send (shttp_connection_t* conn, void *act) {
  usr_context_t       *ctx;

  ctx = (usr_context_t*)act;

  switch(ctx->status) {
  case 0:
    shttp_response_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
    shttp_response_write_header(conn, "abc", 3, "abc", 3);
    shttp_response_write(conn, HTTP_HELLO_WORLD.str, HTTP_HELLO_WORLD.len-3, &on_message_send, act);
  case 1:
    shttp_response_write(conn, HTTP_HELLO_WORLD.str - (HTTP_HELLO_WORLD.len-3), 3, &on_message_send, act);
  case 2:
    shttp_response_end(conn);
  }
}

int  on_message_complete (shttp_connection_t* conn) {
  shttp_response_start(conn, 200, HTTP_CONTENT_TEXT_HTML.str, HTTP_CONTENT_TEXT_HTML.len);
  shttp_response_write(conn, HTTP_HELLO_WORLD.str, HTTP_HELLO_WORLD.len, nil, nil);
  shttp_response_end(conn);
  return 0;
}

int main(int argc, char** argv) {
  shttp_settings_t settings;
  shttp_t          *srv;
  shttp_res        rc;

  HTTP_CONTENT_TEXT_HTML.len = strlen(HTTP_CONTENT_TEXT_HTML.str);
  HTTP_HELLO_WORLD.len = strlen(HTTP_HELLO_WORLD.str);

  memset(&settings, 0, sizeof(shttp_settings_t));
  settings.user_ctx_size = sizeof(usr_context_t);
  shttp_set_log_callback(&on_log);

  settings.callbacks.on_message_begin = &on_message_begin;
  settings.callbacks.on_body = &on_body;
  settings.callbacks.on_message_complete = &on_message_complete;
  srv = shttp_create(&settings);
  if(nil == srv) {
    return -1;
  }
  rc = shttp_listen_at(srv, "tcp4", "0.0.0.0", 8000);
  if(SHTTP_RES_OK != rc) {
    return -1;
  }
  rc = shttp_run(srv);
  if(SHTTP_RES_OK != rc) {
    return -1;
  }
  return 0;
}
