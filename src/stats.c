#include "squirrel_config.h"
#include "internal.h"
#include "uv.h"
#include "../deps/uv/src/uv-common.h"



#ifdef __cplusplus
extern "C" {
#endif

#define MEMORY_INSUFFICIENT    "memory insufficient."

void print_handlers(shttp_connection_internal_t* conn) {
  const char                           *type;
  QUEUE                                *q;
  uv_handle_t                          *h;
  shttp_connection_t                   *external;

  external = &conn_external(conn);
  shttp_response_write_copy(external, "handlers:\r\n", 11);
  QUEUE_FOREACH(q, & external->http->uv_loop->handle_queue) {
    h = QUEUE_DATA(q, uv_handle_t, handle_queue);


    switch (h->type) {
#define X(uc, lc) case UV_##uc: type = #lc; break;
      UV_HANDLE_TYPE_MAP(X)
#undef X
    default:
      type = "<unknown>";
    }
    shttp_response_format(external, "[%c%c%c] %-8s %p\n",
                    "R-"[!(h->flags & UV__HANDLE_REF)],
                    "A-"[!(h->flags & UV__HANDLE_ACTIVE)],
                    "I-"[!(h->flags & UV__HANDLE_INTERNAL)],
                    type,
                    (void*)h);
  }
}

void print_connections(shttp_connection_internal_t* conn) {
  const char                           *type;
  shttp_connection_t                   *external;
  shttp_connection_internal_t          *c;

  external = &conn_external(conn);
  shttp_response_write_copy(external, "connections:\r\n", 14);

  TAILQ_FOREACH(c, &external->http->connections, next) {
    
    switch (c->uv_handle.type) {
#define X(uc, lc) case UV_##uc: type = #lc; break;
      UV_HANDLE_TYPE_MAP(X)
#undef X
    case UV_FILE:
      type = "file";
    default:
      type = "<unknown>";
    }

    shttp_response_format(external, "%p (handle=\"%p, [%c%c%c] %s\", status=%s, request_status=%s)\r\n",
                    (void*)c,
                    (void*)&(c->uv_handle),
                    (0 == uv_has_ref((uv_handle_t*)&c->uv_handle))?'-':'R',
                    (0 == uv_is_active((uv_handle_t*)&c->uv_handle))?'-':'A',
                    (0 == uv_is_closing((uv_handle_t*)&c->uv_handle))?'-':'C',
                    type,
                    _shttp_connection_status_text(conn->status),
                    _shttp_request_status_text(conn_incomming(conn).status));
  }
}

void print_listenings(shttp_connection_internal_t* conn) {
  shttp_connection_t                   *external;
  shttp_listening_t                    *c;

  external = &conn_external(conn);
  shttp_response_write_copy(external, "listenings:\r\n", 13);

  TAILQ_FOREACH(c, &conn->external.http->listenings, next) {
    shttp_response_format(external, "%p [handle=%p]\r\n",
                    (void*)c,
                    (void*)&(c->uv_handle));
  }
}

int stats_handler(shttp_connection_t* external) {
  shttp_connection_internal_t          *conn;

  conn = (shttp_connection_internal_t*)external->internal;

  shttp_response_start(external, 200, HTTP_CONTENT_TEXT.str, HTTP_CONTENT_TEXT.len);
  print_handlers(conn);
  shttp_response_write_copy(external, "\r\n", 2);
  print_listenings(conn);
  shttp_response_write_copy(external, "\r\n", 2);
  print_connections(conn);
  shttp_response_end(external);
  return 0;
}

#ifdef __cplusplus
};
#endif