
#include "squirrel_config.h"

#include <assert.h>

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * HTTP Server Callback Functions
 ****************************************************************************/
static void _shttp_connection_on_data_writed(uv_write_t* req, int status);
static void _shttp_connection_on_head_writed(uv_write_t* req, int status);
static void _shttp_connection_on_disconnect(uv_handle_t* handle);

static int _shttp_connection_start_read(shttp_connection_internal_t* conn);
static int _shttp_connection_stop_read(shttp_connection_internal_t* conn);

  

static void _shttp_on_null_disconnect(uv_handle_t* server_handle) {
  sl_free(server_handle);
}

static void _shttp_on_null_connect(uv_stream_t* server_handle, int status) {
  int          rc;
  uv_handle_t *handle;

  switch (server_handle->type) {
  case UV_TCP:
    handle = (uv_handle_t *)sl_calloc(1, sizeof(uv_tcp_t));
    uv_tcp_init(server_handle->loop, (uv_tcp_t *)handle);
    break;
  case UV_NAMED_PIPE:
    handle = (uv_handle_t *)sl_calloc(1, sizeof(uv_pipe_t));
    uv_pipe_init(server_handle->loop, (uv_pipe_t *)handle, 0);
    break;
  default:
    assert(server_handle->type != UV_TCP);
    return;
  }

  rc = uv_accept(server_handle, (uv_stream_t*)handle);
  if(rc) {
    ERR("accept: %s.", uv_strerror(rc));
    goto end;
  }
  uv_close(handle, &_shttp_on_null_disconnect);
end:
  sl_free(handle);
}


static inline void _shttp_connection_assert(shttp_t* http, shttp_connection_internal_t* conn) {
  assert(conn_external(conn).http == http);
  assert(conn_external(conn).internal == conn);
  assert(conn_external(conn).pool == &conn->pool);
  assert(conn->callbacks == &http->settings.callbacks);
}

static inline void _shttp_connection_assert_external(shttp_t* http, shttp_connection_internal_t* conn) {
  _shttp_connection_assert(http, conn);

  assert(((char*)conn_outgoing(conn).headers.array) == (((char*)conn) + 
                        shttp_mem_align(sizeof(shttp_connection_internal_t)) +
                        http->settings.user_ctx_size +
                        (sizeof(shttp_kv_t) * http->settings.max_headers_count)));
  assert(conn_outgoing(conn).headers.capacity == http->settings.max_headers_count);

  assert(shttp_head_write_buffers_size == conn_outgoing(conn).head_write_buffers.capacity);
  assert(shttp_body_write_buffers_size == conn_outgoing(conn).body_write_buffers.capacity);
  assert(shttp_write_cb_buffers_size == conn_outgoing(conn).call_after_data_writed.capacity);
  assert(shttp_write_cb_buffers_size == conn_outgoing(conn).call_after_completed.capacity);

  
  assert(0 == conn_outgoing(conn).call_after_data_writed.length);
  assert(0 == conn_outgoing(conn).call_after_completed.length);
  assert(0 == conn_outgoing(conn).content_type.len);
  assert(nil == conn_outgoing(conn).content_type.str);
  assert(0 == conn_outgoing(conn).head_buffer.len);
  assert(nil == conn_outgoing(conn).head_buffer.str);
  assert(0 == conn_outgoing(conn).head_buffer.capacity);
}

static inline void _shttp_connection_recycle_request_and_response(shttp_connection_internal_t* conn) {
  memset(&conn_request(conn), 0, sizeof(shttp_request_t));
  memset(&conn_response(conn), 0, sizeof(shttp_response_t));

  _shttp_connection_assert_external(conn_external(conn).http, conn);

  conn_outgoing(conn).write_req.data = nil;
  conn_outgoing(conn).head_write_buffers.length = 0;
  conn_outgoing(conn).body_write_buffers.length = 0;
  conn_outgoing(conn).call_after_completed.length = 0;
  conn_outgoing(conn).call_after_data_writed.length = 0;
  conn_outgoing(conn).is_body_end = 0;
  conn_outgoing(conn).failed_code = 0;
  conn_outgoing(conn).reserved = 0;

  conn_response(conn).headers.array = conn_outgoing(conn).headers.array;
  conn_response(conn).status_code = SHTTP_STATUS_OK;
  conn_response(conn).chunked = SHTTP_THUNKED_NONE;
  conn_response(conn).close_connection = SHTTP_CLOSE_CONNECTION_NONE;
}

static void _shttp_connection_on_disconnect(uv_handle_t* handle) {
  shttp_connection_internal_t *conn;
  shttp_t            *http;

  conn = (shttp_connection_internal_t*)handle->data;
  http = conn_external(conn).http;

  TAILQ_REMOVE(&http->connections, conn, next);
  TAILQ_INSERT_TAIL(&http->free_connections, conn, next);
}

static void _shttp_connection_on_head_writed(uv_write_t* req, int status) {
  shttp_connection_internal_t *conn;
  
  conn = (shttp_connection_internal_t*)req->handle->data;
  if (status < 0) {
    ERR("write: %s", uv_strerror(status));
    conn_outgoing(conn).failed_code = SHTTP_RES_UV;
    _shttp_response_call_hooks_for_failed(conn);
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return;
  }
  conn_outgoing(conn).head_write_buffers.length = 0;
  _shttp_response_send_ready_data(conn,
    &_shttp_connection_on_disconnect, 
    &_shttp_connection_on_head_writed, 
    &_shttp_connection_on_data_writed);
}

static void _shttp_connection_on_data_writed(uv_write_t* req, int status) {
  shttp_connection_internal_t *conn;
  int res;
  
  conn = (shttp_connection_internal_t*)req->handle->data;
  if (status < 0) {
    ERR("write: %s", uv_strerror(status));
    conn_outgoing(conn).failed_code = SHTTP_RES_UV;
    _shttp_response_call_hooks_for_failed(conn);
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return;
  }
  
  conn_outgoing(conn).body_write_buffers.length = 0;
  if(0 != conn_outgoing(conn).is_body_end) {
    _shttp_response_call_hooks_after_writed(conn);
    _shttp_response_call_hooks_after_completed(conn);
    _shttp_response_assert_after_response_end(conn);
  
    res = _shttp_connection_start_read(conn);
    if(0 != res){
      ERR("read_start: %d.", uv_strerror(res));
      conn_outgoing(conn).failed_code = SHTTP_RES_UV;
      uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    }
    return;
  } 
  
  _shttp_response_call_hooks_after_writed(conn);
  
  _shttp_response_send_ready_data(conn,
    &_shttp_connection_on_disconnect, 
    &_shttp_connection_on_head_writed, 
    &_shttp_connection_on_data_writed);
}

int _shttp_connection_on_message_begin(shttp_connection_internal_t* conn) {
  _shttp_connection_recycle_request_and_response(conn);
  
  conn->headers_bytes_count = 0;
  conn->head_reading = 1;
  return conn->callbacks->on_message_begin(&conn_external(conn));
}

int _shttp_connection_on_body (shttp_connection_internal_t* conn, const char *at, size_t length) {
  return conn->callbacks->on_body(&conn_external(conn), at, length);
}

int _shttp_connection_on_message_complete (shttp_connection_internal_t* conn) {
  int        res;
  spool_init(&conn->pool, ((char*)conn->arena_base) + conn->arena_offset,
    conn->arena_capacity - conn->arena_offset);
  
  assert(shttp_message_request_parse_end == conn->status);
  conn->status = shttp_message_response_begin;
  res = conn->callbacks->on_message_complete(&conn_external(conn));
  if(0 != res) {
    ERR("callback: result of callback is %d.", res);
    conn_outgoing(conn).failed_code = res;
    _shttp_response_call_hooks_for_failed(conn);
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return 0;
  }
  res = _shttp_connection_stop_read(conn);
  if(0 != res) {
    ERR("read_stop: %d.", uv_strerror(res));
    conn_outgoing(conn).failed_code = SHTTP_RES_UV;
    _shttp_response_call_hooks_for_failed(conn);
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return 0;
  }

  _shttp_response_send_ready_data(conn,
    &_shttp_connection_on_disconnect, 
    &_shttp_connection_on_head_writed, 
    &_shttp_connection_on_data_writed);
  return 0;
}

static void _shttp_on_connection_alloc(uv_handle_t* req, size_t suggested_size, uv_buf_t* buf) {
  shttp_connection_internal_t *inner;
  size_t                      offset;

  inner = (shttp_connection_internal_t*)req->data;
  offset = inner->arena_offset;

  if(offset == inner->arena_capacity) {
    buf->base = 0;
    buf->len = 0;
    return;
  }
  buf->base = ((char*)inner->arena_base) + offset;
  buf->len = (unsigned long)(inner->arena_capacity - offset);
}

static void _shttp_on_connection_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) {
  size_t                      parsed;
  shttp_connection_internal_t *conn;

  conn = (shttp_connection_internal_t*)tcp->data;
  if (nread <= 0) {
    // No Data Read... Handle Error case
    if (0 != nread && UV_EOF != nread) {
      ERR("read: %s", uv_strerror((int)nread));
    }
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return;
  }


  parsed = http_parser_execute(&conn->parser, &conn_external(conn).http->parser_settings, buf->base, nread);
  if (nread != parsed) {
    if(HPE_OK != HTTP_PARSER_ERRNO(&conn->parser)) {
      ERR("parse error:%s", http_errno_name(HTTP_PARSER_ERRNO(&conn->parser)));
    } else {
      ERR("parse error: parsed is 0.");
    }
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return;
  }
  if (0 == conn->head_reading) {
    if (0 == conn->headers_bytes_count) {
      conn->arena_offset += nread;
    } else {
      conn->arena_offset = shttp_align(conn->headers_bytes_count, shttp_cacheline_size);
    }
  }
  return;
}

void _shttp_on_connect(uv_stream_t* server_handle, int status) {
  int                         rc;
  shttp_listening_t           *listening;
  shttp_connection_internal_t *conn;
  shttp_t                     *http;

  if(-1 == status) {
    ERR("accept: %s", uv_strerror(status));
    return;
  }

  listening = (shttp_listening_t *)server_handle->data;
  http = listening->http;
  if(TAILQ_EMPTY(&http->free_connections)) {
    _shttp_on_null_connect(server_handle, status);
    return ;
  }

  conn = TAILQ_LAST(&http->free_connections, shttp_connections_s);
  conn->arena_offset = 0;
  
  // init uv_handle
  uv_tcp_init(http->uv_loop, &conn->uv_handle);
  conn->uv_handle.data = conn;
  
  // init http parser
  conn->status = shttp_message_request_parse_begin;
  http_parser_init(&conn->parser, HTTP_REQUEST);
  conn->parser.data = conn;

  
  _shttp_connection_assert(http, conn);
  rc = uv_accept(server_handle, (uv_stream_t*)&conn->uv_handle);
  if(rc) {
    ERR("accept: %s\n", uv_strerror(rc));
    uv_close((uv_handle_t*)&conn->uv_handle, &_shttp_connection_on_disconnect);
    return ;
  }
  TAILQ_REMOVE(&http->free_connections, conn, next);
  TAILQ_INSERT_TAIL(&http->connections, conn, next);

  rc = uv_read_start((uv_stream_t*)&conn->uv_handle,
                     _shttp_on_connection_alloc,
                     _shttp_on_connection_read);

  if(rc) {
    ERR("accept: %s", uv_strerror(rc));
    uv_close((uv_handle_t*)&conn->uv_handle, &_shttp_connection_on_disconnect);
    return ;
  }

  return ;
}

int _shttp_connection_stop_read(shttp_connection_internal_t* conn) {
  return uv_read_stop((uv_stream_t*)&conn->uv_handle);
}

int _shttp_connection_start_read(shttp_connection_internal_t* conn) {
  return uv_read_start((uv_stream_t*)&conn->uv_handle,
                     _shttp_on_connection_alloc,
                     _shttp_on_connection_read);
}

#ifdef __cplusplus
};
#endif