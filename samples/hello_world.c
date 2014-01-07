
#include "shttp.h"
#include <string.h>

#define HELLO_WORLD "<html><body>HELLO WORLD!!!</body></html>"

static cstring_t HTTP_CONTENT_TEXT_HTML = { "text/html", 9};

void on_log(int severity, const char *fmt, va_list ap) {
  _vscprintf(fmt, ap);
}

int  on_message_begin (shttp_connection_t*) {
  return 0;
}

int  on_body (shttp_connection_t*, const char *at, size_t length) {
  return 0;
}

typedef struct usr_context_s {
  shttp_connection_t* conn;
  int                 status;
} usr_context_t;


int  on_message_send (void *act) {
  usr_context_t *ctx = (usr_context_t*)act;
  shttp_connection_t* conn = ctx->conn;

  switch(ctx->status) {
  case 0:
    shttp_response_start(conn, 200, &HTTP_CONTENT_TEXT_HTML);
    shttp_response_set_header(conn, "abc", 3, "abc", 3, nil, nil);
    shttp_response_write(conn, "abc", 3, &on_message_send, act);
  case 1:
    shttp_response_write(conn, "abc", 3, &on_message_send, act);
  case 2:
    shttp_response_end(conn);
  }
}

int  on_message_complete (shttp_connection_t* conn) {
  usr_context_t *ctx = (usr_context_t*)conn->ctx;
  ctx->status = 0;
  ctx->conn = conn;
  return on_message_send(ctx);
}

int main(int argc, char** argv) {
  shttp_settings_t settings;
  shttp_t          *srv;
  shttp_res        rc;
  
  memset(&settings, 0, sizeof(shttp_settings_t));
  settings.user_ctx_size = sizeof(usr_context_t);
  shttp_set_log_callback(&on_log);
  srv = shttp_create(nil);
  if(nil == srv) {
    return -1;
  }
  rc = shttp_listen_at(srv, "0.0.0.0", 8000);
  if(SHTTP_RES_OK != rc) {
    return -1;
  }
  rc = shttp_run(srv);
  if(SHTTP_RES_OK != rc) {
    return -1;
  }
  return 0;
}
