
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
  shttp_t                     *http = nil;
  shttp_connection_internal_t *conn = nil;
  shttp_context_internal_t    *ctx = nil;
  char                        *ptr = nil;
  size_t                      i = 0;

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

  if (NULL == settings || settings->connection_user_ctx_size < 0) {
    http->settings.connection_user_ctx_size = SHTTP_CONNECTION_USER_CTX_SIZE;
  } else {
    http->settings.connection_user_ctx_size = shttp_mem_align(settings->connection_user_ctx_size);
  }

  if (NULL == settings || settings->context_user_ctx_size < 0) {
    http->settings.context_user_ctx_size = SHTTP_CONTEXT_USER_CTX_SIZE;
  } else {
    http->settings.context_user_ctx_size = shttp_mem_align(settings->context_user_ctx_size);
  }

  if (NULL == settings || settings->rw_buffer_size <= 0) {
    http->settings.rw_buffer_size = SHTTP_RW_BUFFER_SIZE;
  } else {
    http->settings.rw_buffer_size = shttp_align(settings->rw_buffer_size, shttp_pagesize);
  }

  TAILQ_INIT(&http->connections);
  TAILQ_INIT(&http->free_connections);
  TAILQ_INIT(&http->listenings);
  TAILQ_INIT(&http->free_contexts);

  memcpy(&http->settings.callbacks, &settings->callbacks, sizeof(shttp_callbacks_t));
  http->uv_loop = uv_loop_new();


  _shttp_parser_init(&http->parser_settings);

  for(i = 0; i < http->settings.max_connections_size; ++i) {
    ptr = (char*)sl_calloc(1, shttp_mem_align(sizeof(shttp_connection_internal_t)) +
                           http->settings.connection_user_ctx_size);
    conn = (shttp_connection_internal_t*)ptr;
    conn->external.http = http;
    conn->external.internal = conn;
    conn->external.pool = &conn->pool;
    TAILQ_INIT(&conn->contexts);

    if(0 == http->settings.connection_user_ctx_size) {
      conn->external.ctx = nil;
    } else {
      conn->external.ctx = ptr + shttp_mem_align(sizeof(shttp_connection_internal_t));
    }
    conn->callbacks = &http->settings.callbacks;

    TAILQ_INSERT_TAIL(&http->free_connections, conn, next);
  }

  
  for(i = 0; i < (2*http->settings.max_connections_size); ++i) {
    
    ptr = (char*)sl_calloc(1, shttp_mem_align(sizeof(shttp_context_internal_t)) +
                           http->settings.context_user_ctx_size +
                           (2 * sizeof(shttp_kv_t) * http->settings.max_headers_count) +
                           http->settings.rw_buffer_size);

    
    ctx = (shttp_context_internal_t*)ptr;
    if(0 == http->settings.context_user_ctx_size) {
      conn->external.ctx = nil;
    } else {
      conn->external.ctx = ptr + shttp_mem_align(sizeof(shttp_context_internal_t));
    }

    ctx_incomming(ctx).headers.array = (shttp_kv_t*) (ptr + shttp_mem_align(sizeof(shttp_context_internal_t)) +
                                    http->settings.context_user_ctx_size +
                                   (2 * sizeof(shttp_kv_t) * http->settings.max_headers_count)+ 
                                   http->settings.rw_buffer_size);
    ctx_incomming(ctx).headers.capacity = http->settings.max_headers_count;
    ctx_incomming(ctx).headers.length = 0;


    ctx_outgoing(ctx).headers.array = (shttp_kv_t*) (ptr + shttp_mem_align(sizeof(shttp_context_internal_t)) +
                                   http->settings.context_user_ctx_size +
                                   (sizeof(shttp_kv_t) * http->settings.max_headers_count));
    ctx_outgoing(ctx).headers.capacity = http->settings.max_headers_count;
    //ctx_outgoing(ctx).headers.length = 0;
    ctx_outgoing(ctx).head_write_buffers.capacity = shttp_head_write_buffers_size;
    ctx_outgoing(ctx).head_write_buffers.length = 0;
    ctx_outgoing(ctx).call_after_completed.capacity = shttp_write_cb_buffers_size;
    ctx_outgoing(ctx).call_after_completed.length = 0;
    ctx_outgoing(ctx).body_write_buffers.capacity = shttp_body_write_buffers_size;
    ctx_outgoing(ctx).body_write_buffers.length = 0;
    ctx_outgoing(ctx).call_after_data_writed.capacity = shttp_write_cb_buffers_size;
    ctx_outgoing(ctx).call_after_data_writed.length = 0;

    ctx->arena_base = ptr + shttp_mem_align(sizeof(shttp_connection_internal_t)) +
                       http->settings.connection_user_ctx_size +
                       (2 * sizeof(shttp_kv_t) * http->settings.max_headers_count);
    ctx->arena_capacity = http->settings.rw_buffer_size;
    TAILQ_INSERT_TAIL(&http->free_contexts, ctx, next);
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

  rc = uv_tcp_bind(&listening->uv_handle, addr);
  UV_CHECK(rc, http->uv_loop, "bind");

  rc = uv_listen((uv_stream_t*)&listening->uv_handle, 128, &_shttp_on_connect);
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

static void _shttp_on_listening_close(uv_handle_t* handle) {
  shttp_listening_t *listening;
  shttp_t           *http;

  listening = (shttp_listening_t *)handle->data;
  http = listening->http;

  TAILQ_REMOVE(&http->listenings, listening, next);
  sl_free(listening);
}

static void _shttp_close_shutdown_request(uv_handle_t* handle) {
  sl_free((uv_async_t*)handle);
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

DLL_VARIABLE shttp_res shttp_shutdown(shttp_t *http) {
  uv_async_t *async;
  int        rc;

  async = (uv_async_t*)sl_malloc(sizeof(uv_async_t));
  rc = uv_async_init(http->uv_loop, async, &_shttp_shutdown);
  if(0 != rc) {
    ERR("shttp_shutdown: %s", uv_strerror(rc));
    sl_free(async);
    return SHTTP_RES_UV;
  }
  async->data = http;
  rc = uv_async_send(async);
  if(0 != rc) {
    ERR("shttp_shutdown: %s", uv_strerror(rc));
    sl_free(async);
    return SHTTP_RES_UV;
  }
}

#ifdef __cplusplus
};
#endif