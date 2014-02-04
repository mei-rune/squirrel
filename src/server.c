
#include "squirrel_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

void _shttp_parser_init(struct http_parser_settings *parser_settings);
void _shttp_connection_on_connect(uv_stream_t* server_handle, int status);

/*****************************************************************************
 * HTTP Server API
 ****************************************************************************/

cstring_t HTTP_CONTENT_HTML = { 9, "text/html"};
cstring_t HTTP_CONTENT_TEXT = { 10, "text/plain"};


static void _shttp_on_listening_close(uv_handle_t* handle) {
  shttp_listening_t *listening;
  shttp_t           *http;

  listening = (shttp_listening_t *)handle->data;
  http = listening->http;

  TAILQ_REMOVE(&http->listenings, listening, next);
  sl_free(listening);
}

static void _shttp_close_shutdown_request(uv_handle_t* handle) {
}

static void _shttp_shutdown(uv_async_t* handle, int status) {
  shttp_listening_t  *listening;
  shttp_t            *http;

  http = (shttp_t*)handle->data;
  TAILQ_FOREACH(listening, &http->listenings, next) {
    uv_close((uv_handle_t *)&listening->uv_handle, &_shttp_on_listening_close);
  }
  uv_close((uv_handle_t *)handle, &_shttp_close_shutdown_request);
}

DLL_VARIABLE shttp_t *shttp_create(shttp_settings_t *settings) {
  shttp_t                     *http = nil;
  shttp_connection_internal_t *conn = nil;
  char                        *ptr;
  size_t                      i = 0;

  os_init();

  http = (shttp_t*)sl_calloc(1, sizeof(shttp_t));

  if (nil == http) {
    ERR("calloc failed.");
    return (nil);
  }

  if (nil == settings) {
    http->settings.timeout.tv_sec = 0;
    http->settings.timeout.tv_usec = 0;
  } else {
    memcpy(&http->settings.timeout, &settings->timeout, sizeof(struct timeval));
  }

  if (nil == settings || settings->max_headers_count <= 0) {
    http->settings.max_headers_count = SHTTP_MAX_HEADER_COUNT;
  } else {
    http->settings.max_headers_count = settings->max_headers_count;
  }

  if (nil == settings || settings->max_body_size <= 0) {
    http->settings.max_body_size = UINT64_MAX;
  } else {
    http->settings.max_body_size = settings->max_body_size;
  }

  if (nil == settings || settings->allowed_methods <= 0) {
    http->settings.allowed_methods = SHTTP_REQ_GET | SHTTP_REQ_POST |
                                     SHTTP_REQ_HEAD | SHTTP_REQ_PUT | SHTTP_REQ_DELETE;
  } else {
    http->settings.allowed_methods = settings->allowed_methods;
  }

  if (nil == settings || settings->max_connections_size <= 0) {
    http->settings.max_connections_size = SHTTP_MAX_CONNECTIONS;
  } else {
    http->settings.max_connections_size = settings->max_connections_size;
  }

  if (nil == settings || settings->user_ctx_size < 0) {
    http->settings.user_ctx_size = SHTTP_CTX_SIZE;
  } else {
    http->settings.user_ctx_size = shttp_mem_align(settings->user_ctx_size);
  }

  if (nil == settings || settings->rw_buffer_size <= 0) {
    http->settings.rw_buffer_size = SHTTP_RW_BUFFER_SIZE;
  } else {
    http->settings.rw_buffer_size = shttp_align(settings->rw_buffer_size, shttp_pagesize);
  }

  TAILQ_INIT(&http->connections);
  TAILQ_INIT(&http->free_connections);
  TAILQ_INIT(&http->listenings);

  memcpy(&http->settings.callbacks, &settings->callbacks, sizeof(shttp_callbacks_t));
  http->uv_loop = uv_loop_new();
  if(nil == http->uv_loop) {
    ERR("uv_loop_new failed.");
    return (nil);
  }
  if(0 != uv_async_init(http->uv_loop, &http->shutdown_signal, &_shttp_shutdown)) {
    ERR("uv_async_init failed.");
    return (nil);
  }
  http->shutdown_signal.data = http;


  _shttp_parser_init(&http->parser_settings);

  for(i = 0; i < http->settings.max_connections_size; ++i) {
    ptr = (char*)sl_calloc(1,
                           shttp_mem_align(sizeof(shttp_connection_internal_t)) +
                           shttp_mem_align(http->settings.user_ctx_size) +
                           (2 * sizeof(shttp_kv_t) * http->settings.max_headers_count) +
                           http->settings.rw_buffer_size);
    conn = (shttp_connection_internal_t*)ptr;
    conn_external(conn).http = http;
    conn_external(conn).internal = conn;
    conn_external(conn).pool = &conn->pool;

    if(0 == http->settings.user_ctx_size) {
      conn_external(conn).ctx = nil;
    } else {
      conn_external(conn).ctx = ptr + shttp_mem_align(sizeof(shttp_connection_internal_t));
    }
    conn->callbacks = &http->settings.callbacks;


    // init connection.incomming
    conn_incomming(conn).headers.array = (shttp_kv_t*) (ptr + shttp_mem_align(sizeof(shttp_connection_internal_t)) +
                                         shttp_mem_align(http->settings.user_ctx_size));
    conn_incomming(conn).headers.capacity = http->settings.max_headers_count;
    conn_incomming(conn).headers.length = 0;

    // init connection.outgoing
    conn_outgoing(conn).headers.array = (shttp_kv_t*) (ptr + shttp_mem_align(sizeof(shttp_connection_internal_t)) +
                                        shttp_mem_align(http->settings.user_ctx_size) +
                                        (sizeof(shttp_kv_t) * http->settings.max_headers_count));
    conn_outgoing(conn).headers.capacity = http->settings.max_headers_count;
    //conn_outgoing(conn).headers.length = 0;
    conn_outgoing(conn).head_write_buffers.capacity = shttp_head_write_buffers_size;
    conn_outgoing(conn).head_write_buffers.length = 0;
    conn_outgoing(conn).body_write_buffers.capacity = shttp_body_write_buffers_size;
    conn_outgoing(conn).body_write_buffers.length = 0;
    conn_outgoing(conn).call_after_data_writed.capacity = shttp_write_cb_buffers_size;
    conn_outgoing(conn).call_after_data_writed.length = 0;
    conn_outgoing(conn).call_after_completed.capacity = shttp_write_cb_buffers_size;
    conn_outgoing(conn).call_after_completed.length = 0;

    conn->arena_base = ptr + shttp_align(shttp_mem_align(sizeof(shttp_connection_internal_t)) +
                                         http->settings.user_ctx_size, shttp_pagesize) +
                       (2 * sizeof(shttp_kv_t) * http->settings.max_headers_count);
    conn->arena_capacity = http->settings.rw_buffer_size;

    TAILQ_INSERT_TAIL(&http->free_connections, conn, next);
  }

  return (http);
}

DLL_VARIABLE void shttp_free(shttp_t *http) {
  shttp_connection_internal_t *conn;

  assert(TAILQ_EMPTY(&http->listenings));
  assert(TAILQ_EMPTY(&http->connections));
  while(!TAILQ_EMPTY(&http->free_connections)) {
    conn = TAILQ_FIRST(&http->free_connections);
    TAILQ_REMOVE(&http->free_connections, conn, next);
    sl_free(conn);
  }

  uv_loop_delete(http->uv_loop);
  sl_free(http);
}

DLL_VARIABLE shttp_res shttp_listen_at(shttp_t* http, const char *network, char *listen_addr, short port) {
  int rc;
  shttp_listening_t   *listening;
  struct sockaddr_in  address4;
  struct sockaddr_in6 address6;
  struct sockaddr     *addr;


  if(nil == network || 0 == strlen(network) ||
      0 == strcasecmp("tcp", network) || 0 == strcasecmp("tcp4", network)) {
    rc = uv_ip4_addr(listen_addr, port, &address4);
    UV_CHECK(rc, http->uv_loop, "inet_pton");
    addr = (struct sockaddr *)&address4;
  } else if(0 == strcasecmp("tcp6", network)) {
    rc = uv_ip6_addr(listen_addr, port, &address6);
    UV_CHECK(rc, http->uv_loop, "inet_pton");
    addr = (struct sockaddr *)&address6;
  } else {
    ERR("listen_at: '%s' is unknown.", network);
    return SHTTP_RES_ERROR;
  }

  listening = (shttp_listening_t *)sl_calloc(1, sizeof(shttp_listening_t));
  listening->http = http;

  rc = uv_tcp_init(http->uv_loop, &listening->uv_handle);
  UV_CHECK(rc, http->uv_loop, "init");
  listening->uv_handle.data = listening;

  rc = uv_tcp_bind(&listening->uv_handle, addr, 0);
  UV_CHECK(rc, http->uv_loop, "bind");

  rc = uv_listen((uv_stream_t*)&listening->uv_handle, 128, &_shttp_connection_on_connect);
  UV_CHECK(rc, http->uv_loop, "listening");

  TAILQ_INSERT_TAIL(&http->listenings, listening, next);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_run(shttp_t *http) {
  int rc;
  rc = uv_run(http->uv_loop, UV_RUN_DEFAULT);
  UV_CHECK(rc, http->uv_loop, "listening");
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_shutdown(shttp_t *http) {
  int        rc;

  rc = uv_async_send(&http->shutdown_signal);
  if(0 != rc) {
    ERR("shttp_shutdown: %s", uv_strerror(rc));
    return SHTTP_RES_UV;
  }
  return SHTTP_RES_OK;
}

DLL_VARIABLE void shttp_set_context(shttp_t *http, void *ctx) {
  http->context = ctx;
}

DLL_VARIABLE void * shttp_get_context(shttp_t *http) {
  return http->context;
}

#ifdef __cplusplus
};
#endif