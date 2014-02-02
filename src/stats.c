#include "squirrel_config.h"
#include "internal.h"
#include "uv.h"
#include "../deps/uv/src/uv-common.h"



#ifdef __cplusplus
extern "C" {
#endif

#define MEMORY_INSUFFICIENT    "memory insufficient."

void print_handlers(shttp_connection_internal_t* conn) {
  char                                 *p;
  size_t                               len, capacity;
  const char                           *type;
  QUEUE                                *q;
  uv_handle_t                          *h;
  shttp_connection_t                   *external;

  external = &conn_external(conn);
  p = (char*)shttp_response_mem_malloc(external, 2*1024);
  if(nil == p) {
    shttp_response_write(external, MEMORY_INSUFFICIENT, strlen(MEMORY_INSUFFICIENT), nil, nil);
    return;
  }
  capacity = 2*1024;
  len = 0;

  shttp_response_write(external, "handlers:\r\n", 11, nil, nil);
  QUEUE_FOREACH(q, & external->http->uv_loop->handle_queue) {
    h = QUEUE_DATA(q, uv_handle_t, handle_queue);


    switch (h->type) {
#define X(uc, lc) case UV_##uc: type = #lc; break;
      UV_HANDLE_TYPE_MAP(X)
#undef X
    default:
      type = "<unknown>";
    }
    if(len < 128) {
      shttp_response_write(external, p, len, &shttp_response_pool_free, p);
      p = (char*)shttp_response_mem_malloc(external, 2*1024);
      if(nil == p) {
        shttp_response_write(external, MEMORY_INSUFFICIENT, strlen(MEMORY_INSUFFICIENT), nil, nil);
        return;
      }
      capacity = 2*1024;
      len = 0;
    }

    len += snprintf(p + len, capacity - len,
                    "[%c%c%c] %-8s %p\n",
                    "R-"[!(h->flags & UV__HANDLE_REF)],
                    "A-"[!(h->flags & UV__HANDLE_ACTIVE)],
                    "I-"[!(h->flags & UV__HANDLE_INTERNAL)],
                    type,
                    (void*)h);
  }
  if(nil != p) {
    if(0 < len) {
      shttp_response_write(external, p, len, &shttp_response_pool_free, p);
    } else {
      shttp_response_mem_free(external, p);
    }
  }
}

void print_connections(shttp_connection_internal_t* conn) {
  char                                 *p;
  size_t                               len, capacity;
  shttp_connection_t                   *external;
  shttp_connection_internal_t          *c;

  external = &conn_external(conn);

  p = (char*)shttp_response_mem_malloc(external, 2*1024);
  if(nil == p) {
    shttp_response_write(external, MEMORY_INSUFFICIENT, strlen(MEMORY_INSUFFICIENT), nil, nil);
    return;
  }
  capacity = 2*1024;
  len = 0;

  shttp_response_write(external, "connections:\r\n", 14, nil, nil);

  TAILQ_FOREACH(c, &conn->external.http->connections, next) {
    if(len < 128) {
      shttp_response_write(external, p, len, &shttp_response_pool_free, p);
      p = (char*)shttp_response_mem_malloc(external, 2*1024);
      if(nil == p) {
        shttp_response_write(external, MEMORY_INSUFFICIENT, strlen(MEMORY_INSUFFICIENT), nil, nil);
        return;
      }
      capacity = 2*1024;
      len = 0;
    }


    len += snprintf(p + len, capacity - len,
                    "%p [handle=%p]\r\n",
                    (void*)c,
                    (void*)&(c->uv_handle));
  }


  if(nil != p) {
    if(0 < len) {
      shttp_response_write(external, p, len, &shttp_response_pool_free, p);
    } else {
      shttp_response_mem_free(external, p);
    }
  }
}

void print_listenings(shttp_connection_internal_t* conn) {
  char                                 *p;
  size_t                               len, capacity;
  shttp_connection_t                   *external;
  shttp_listening_t                    *c;

  external = &conn_external(conn);

  p = (char*)shttp_response_mem_malloc(external, 2*1024);
  if(nil == p) {
    shttp_response_write(external, MEMORY_INSUFFICIENT, strlen(MEMORY_INSUFFICIENT), nil, nil);
    return;
  }
  capacity = 2*1024;
  len = 0;

  shttp_response_write(external, "listenings:\r\n", 13, nil, nil);

  TAILQ_FOREACH(c, &conn->external.http->listenings, next) {
    if(len < 128) {
      shttp_response_write(external, p, len, &shttp_response_pool_free, p);
      p = (char*)shttp_response_mem_malloc(external, 2*1024);
      if(nil == p) {
        shttp_response_write(external, MEMORY_INSUFFICIENT, strlen(MEMORY_INSUFFICIENT), nil, nil);
        return;
      }
      capacity = 2*1024;
      len = 0;
    }


    len += snprintf(p + len, capacity - len,
                    "%p [handle=%p]\r\n",
                    (void*)c,
                    (void*)&(c->uv_handle));
  }


  if(nil != p) {
    if(0 < len) {
      shttp_response_write(external, p, len, &shttp_response_pool_free, p);
    } else {
      shttp_response_mem_free(external, p);
    }
  }
}

int stats_handler(shttp_connection_t* external) {
  shttp_connection_internal_t          *conn;

  conn = (shttp_connection_internal_t*)external->internal;

  shttp_response_start(external, 200, HTTP_CONTENT_TEXT.str, HTTP_CONTENT_TEXT.len);
  print_handlers(conn);
  shttp_response_write(external, "\r\n", 2, nil, nil);
  print_listenings(conn);
  shttp_response_write(external, "\r\n", 2, nil, nil);
  print_connections(conn);
  shttp_response_end(external);
  return 0;
}


#ifdef __cplusplus
};
#endif