
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
void _shttp_on_connect(uv_stream_t* server_handle, int status);

/*****************************************************************************
 * HTTP Server API
 ****************************************************************************/


DLL_VARIABLE shttp_t *shttp_create(shttp_settings_t *settings) {
  shttp_t *http = NULL;
  shttp_connection_internal_t *conn = NULL;
  size_t i = 0;

  if (0 == shttp_pagesize) {
    os_init();

    if (0 == shttp_pagesize) {
      ERR("Please first call os_init() or os_init() failed.");
      return NULL;
    }
  }


  http = (shttp_t*)sl_calloc(1, sizeof(shttp_t));

  if (NULL == http) {
    ERR("calloc failed.");
    return (NULL);
  }

  if (NULL == settings) {
    http->settings.timeout.tv_sec = 0;
    http->settings.timeout.tv_usec = 0;
  } else {
    memcpy(&http->settings.timeout, &settings->timeout, sizeof(struct timeval));
  }

  if (NULL == settings || settings->max_headers_count <= 0) {
    http->settings.max_headers_count = SHTTP_MAX_HEADER_COUNT;
  } else {
    http->settings.max_headers_count = settings->max_headers_count;
  }

  if (NULL == settings || settings->max_headers_size <= 0) {
    http->settings.max_headers_size = SHTTPE_MAX_HEADER_SIZE;
  } else {
    http->settings.max_headers_size = settings->max_headers_size;
  }

  if (NULL == settings || settings->max_body_size <= 0) {
    http->settings.max_body_size = UINT64_MAX;
  } else {
    http->settings.max_body_size = settings->max_body_size;
  }

  if (NULL == settings || settings->allowed_methods <= 0) {
    http->settings.allowed_methods = SHTTP_REQ_GET | SHTTP_REQ_POST |
                                     SHTTP_REQ_HEAD | SHTTP_REQ_PUT | SHTTP_REQ_DELETE;
  } else {
    http->settings.allowed_methods = settings->allowed_methods;
  }

  if (NULL == settings || settings->max_connections_size <= 0) {
    http->settings.max_connections_size = SHTTP_MAX_CONNECTIONS;
  } else {
    http->settings.max_connections_size = settings->max_connections_size;
  }

  if (NULL == settings || settings->user_ctx_size < 0) {
    http->settings.user_ctx_size = SHTTP_CTX_SIZE;
  } else {
    http->settings.user_ctx_size = shttp_mem_align(settings->user_ctx_size);
  }

  if (NULL == settings || settings->rw_buffer_size <= 0) {
    http->settings.rw_buffer_size = SHTTP_RW_BUFFER_SIZE;
  } else {
    http->settings.rw_buffer_size = shttp_align(settings->rw_buffer_size, shttp_pagesize);
  }

  TAILQ_INIT(&http->connections);
  TAILQ_INIT(&http->free_connections);
  TAILQ_INIT(&http->listenings);

  http->uv_loop = uv_loop_new();


  _shttp_parser_init(&http->parser_settings);

  for(i = 0; i < http->settings.max_connections_size; ++i) {
    conn = (shttp_connection_internal_t*)sl_calloc(1,
           shttp_align(shttp_mem_align(sizeof(shttp_connection_internal_t)) +
                       http->settings.user_ctx_size, shttp_pagesize) +
           (2 * sizeof(shttp_kv_t) * http->settings.max_headers_count) +
           http->settings.rw_buffer_size);

    conn->inner.http = http;
    conn->inner.internal = conn;
    conn->inner.ctx = ((char*)conn) + shttp_mem_align(sizeof(shttp_connection_internal_t));
    conn->callbacks = &http->settings.callbacks;
    
    conn->incomming.headers.array = (shttp_kv_t*) ((char*)conn) + shttp_align(shttp_mem_align(sizeof(shttp_connection_internal_t)) +
                          http->settings.user_ctx_size, shttp_pagesize);
    conn->incomming.headers.capacity = http->settings.max_headers_count;
    conn->incomming.headers.length = 0;
    conn->incomming.conn = conn;

    
    conn->outgoing.headers.array = (shttp_kv_t*) ((char*)conn) + shttp_align(shttp_mem_align(sizeof(shttp_connection_internal_t)) +
                          http->settings.user_ctx_size, shttp_pagesize) +
                          (sizeof(shttp_kv_t) * http->settings.max_headers_count);
    conn->outgoing.headers.capacity = http->settings.max_headers_count;
    conn->outgoing.buffer.capacity = 256;
    conn->outgoing.buffer.length = 0;
    conn->outgoing.write_cb_list.capacity = 2048;
    conn->outgoing.write_cb_list.length = 0;

    conn->arena_base = ((char*)conn) + shttp_align(shttp_mem_align(sizeof(shttp_connection_internal_t)) +
                          http->settings.user_ctx_size, shttp_pagesize) +
                          (2 * sizeof(shttp_kv_t) * http->settings.max_headers_count);
    conn->arena_capacity = http->settings.rw_buffer_size;
    conn->arena_offset = 0;

    TAILQ_INSERT_TAIL(&http->free_connections, conn, next);
  }

  return (http);
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

  rc = uv_tcp_init(http->uv_loop, &listening->uv_handle);
  UV_CHECK(rc, http->uv_loop, "init");
  listening->uv_handle.data = listening;
  
  rc = uv_tcp_bind(&listening->uv_handle, addr);
  UV_CHECK(rc, http->uv_loop, "bind");

  rc = uv_listen((uv_stream_t*)&listening->uv_handle, 128, &_shttp_on_connect);
  UV_CHECK(rc, http->uv_loop, "listening");

  TAILQ_INSERT_TAIL(&http->listenings, listening, next);
  return SHTTP_RES_OK;
}

static void _shttp_free(shttp_t *http) {
  shttp_connection_internal_t *conn;
  //shttp_listening_t  *listening;

  assert(TAILQ_EMPTY(&http->listenings));
  assert(TAILQ_EMPTY(&http->connections));

  TAILQ_FOREACH(conn, &http->free_connections, next) {
    sl_free(conn);
  }

  uv_loop_delete(http->uv_loop);
  sl_free(http);
}

DLL_VARIABLE shttp_res shttp_run(shttp_t *http) {
  int rc;
  rc = uv_run(http->uv_loop, UV_RUN_DEFAULT);
  UV_CHECK(rc, http->uv_loop, "listening");
  return SHTTP_RES_OK;
}

static void _shttp_on_listening_close(uv_handle_t* handle) {
  shttp_listening_t *listening;
  shttp_t           *http;

  listening = (shttp_listening_t *)handle->data;
  http = listening->http;

  TAILQ_REMOVE(&http->listenings, listening, next);
  sl_free(listening);
}

DLL_VARIABLE shttp_res shttp_shutdown(shttp_t *http) {
  shttp_listening_t* listening;

  TAILQ_FOREACH(listening, &http->listenings, next) {
    uv_close((uv_handle_t *)&listening->uv_handle, &_shttp_on_listening_close);
  }
  return SHTTP_RES_OK;
}

#ifdef __cplusplus
};
#endif