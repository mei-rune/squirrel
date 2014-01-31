
#include "squirrel_config.h"

#include <assert.h>

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * HTTP Server Callback Functions
 ****************************************************************************/
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
  return;
end:
  sl_free(handle);
}

#ifdef DEBUG
static inline void _shttp_connection_memory_check(shttp_connection_internal_t* conn) {
  spool_block_t               *block;

  if(nil != conn->rd_buf.s) {
    spool_free(&conn->pool, conn->rd_buf.s);
  }
  if(nil != conn->incomming.headers_block &&
      conn->rd_buf.s != conn->incomming.headers_block) {
    spool_free(&conn->pool, conn->incomming.headers_block);
  }

  if(!TAILQ_EMPTY(&conn->pool.all)) {
    ERR("detect mem leaks:");
    TAILQ_FOREACH(block, &conn->pool.all, all_next) {
      if(block->used) {
        ERR("mem: %s(%d)", block->file, block->line);
      }
    }
  }
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
#else
#define _shttp_connection_memory_check(conn)
#define _shttp_connection_assert(http, conn)
#define _shttp_connection_assert_external(http, conn)
#endif

static void _shttp_connection_parse_request(shttp_connection_internal_t *conn,
    char* base, ssize_t len, size_t nread) {
  size_t                      parsed;
  int                         rc;
  enum http_errno             parse_errno;

  parsed = http_parser_execute(&conn->parser,
                               &conn_external(conn).http->parser_settings, base, len);

  parse_errno = HTTP_PARSER_ERRNO(&conn->parser);

  if(HPE_OK == parse_errno) {
    if (len != parsed) {
      ERR("parse error: nread != parsed.");
      uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
      return;
    }

    if (shttp_headers_is_reading(conn)) {
      conn->rd_buf.end += nread;
      conn->rd_buf.start += parsed;
    }
    return;
  }

  if(HPE_PAUSED != parse_errno) {
    if(HPE_CB_header_field != parse_errno &&
        HPE_CB_header_value != parse_errno) {
      ERR("parse error:%s", http_errno_name(parse_errno));
    }
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return;
  }

  rc = uv_read_stop((uv_stream_t*)&conn->uv_handle);
  if(0 != rc) {
    ERR("read_stop: %s.", uv_strerror(rc));
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return;
  }

  // a request is read completed, response is writing.
  conn->rd_buf.end += nread;
  conn->rd_buf.start += parsed;
}

static inline void _shttp_connection_parse_request_first(shttp_connection_internal_t *conn,
    char* base, ssize_t len, size_t nread) {
  conn_incomming(conn).headers_buf.str = base;
  conn_incomming(conn).headers_buf.len = 0;
  _shttp_connection_parse_request(conn, base, len, nread);
}

static inline void _shttp_connection_restart_read_request(shttp_connection_internal_t* conn) {
  int               rc;

  http_parser_pause(&conn->parser, 0);
  conn->status = shttp_message_request_parse_none;
  if(conn->incomming.headers_block != conn->rd_buf.s) {
    spool_free(&conn->pool, conn->incomming.headers_block);
  }
  conn->incomming.headers_block = nil;

  if(conn->rd_buf.end != conn->rd_buf.start) {
    _shttp_connection_parse_request_first(conn,
                                          conn->rd_buf.s + conn->rd_buf.start,
                                          conn->rd_buf.end - conn->rd_buf.start, 0);

    if(HPE_PAUSED == HTTP_PARSER_ERRNO(&conn->parser)) {
      return;
    }

    if(uv_is_closing((uv_handle_t*)&conn->uv_handle)) {
      return;
    }

  } else {
#ifdef DEBUG
    _shttp_connection_memory_check(conn);
    conn->rd_buf.s = (char*)spool_malloc(&conn->pool, spool_excepted(2*1024));
    conn->rd_buf.capacity = spool_excepted(2*1024);
#endif
    conn->rd_buf.end = 0;
    conn->rd_buf.start = 0;
  }

  rc = uv_read_start((uv_stream_t*)&conn->uv_handle,
                     _shttp_connection_on_alloc,
                     _shttp_connection_on_read);
  if(0 != rc) {
    ERR("read_start: %s.", uv_strerror(rc));
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
  }
}

static void _shttp_connection_on_disconnect(uv_handle_t* handle) {
  shttp_connection_internal_t *conn;
  shttp_t                     *http;

  conn = (shttp_connection_internal_t*)handle->data;
  http = conn_external(conn).http;

  _shttp_connection_memory_check(conn);

  if(nil != conn->callbacks->on_disconnected) {
    if(0 != conn->callbacks->on_disconnected(&conn_external(conn))) {
      ERR("callbacks: on_disconnected is failed.");
    }
  }

  TAILQ_REMOVE(&http->connections, conn, next);
  TAILQ_INSERT_TAIL(&http->free_connections, conn, next);
}

static void _shttp_connection_on_head_writed(uv_write_t* req, int status) {
  shttp_connection_internal_t *conn;

  conn = (shttp_connection_internal_t*)req->handle->data;
  if (status < 0) {
    ERR("write: %s", uv_strerror(status));
    conn_outgoing(conn).failed_code = SHTTP_RES_UV;
    _shttp_response_on_completed(conn, -1);
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

  conn = (shttp_connection_internal_t*)req->handle->data;
  if (status < 0) {
    ERR("write: %s", uv_strerror(status));
    conn_outgoing(conn).failed_code = SHTTP_RES_UV;
    _shttp_response_on_completed(conn, -1);
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return;
  }

  if(0 != conn_outgoing(conn).is_body_end) {
    _shttp_response_on_completed(conn, 0);
    _shttp_connection_restart_read_request(conn);
    return;
  }

  conn_outgoing(conn).body_write_buffers.length = 0;
  _shttp_response_call_hooks_after_writed(conn);

  _shttp_response_send_ready_data(conn,
                                  &_shttp_connection_on_disconnect,
                                  &_shttp_connection_on_head_writed,
                                  &_shttp_connection_on_data_writed);
}

int _shttp_connection_on_message_begin(shttp_connection_internal_t* conn) {
  memset(&conn_request(conn), 0, sizeof(shttp_request_t));
  memset(&conn_response(conn), 0, sizeof(shttp_response_t));

  _shttp_connection_assert_external(conn_external(conn).http, conn);

  conn_incomming(conn).headers_block = nil;
  shttp_set_length(conn_incomming(conn).headers, 0);
  sstring_init(conn_incomming(conn).url, nil, 0);
  //sstring_init(conn_incomming(conn).headers_buf, nil, 0);

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

  return conn->callbacks->on_message_begin(&conn_external(conn));
}

int _shttp_connection_on_body (shttp_connection_internal_t* conn, const char *at, size_t length) {
  return conn->callbacks->on_body(&conn_external(conn), at, length);
}

int _shttp_connection_on_headers_complete(shttp_connection_internal_t* conn) {
  conn_incomming(conn).headers_block = conn->rd_buf.s;
  conn_incomming(conn).headers_buf.len = conn->parser.nread;
  return 0;
}

int _shttp_connection_on_message_complete (shttp_connection_internal_t* conn) {
  int        res;

  if((6 == conn_request(conn).uri.path.len &&
      0 == strncmp("/stats", conn_request(conn).uri.path.str, 6)) ||
      (7 == conn_request(conn).uri.path.len &&
       0 == strncmp("/stats/", conn_request(conn).uri.path.str, 7))) {
    res = stats_handler(&conn_external(conn));
  } else {
    res = conn->callbacks->on_message_complete(&conn_external(conn));
  }
  if(0 != res) {
    ERR("callback: result of callback is %d.", res);
    conn_outgoing(conn).failed_code = res;
    _shttp_response_on_completed(conn, -1);
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return 0;
  }

  _shttp_response_send_ready_data(conn,
                                  &_shttp_connection_on_disconnect,
                                  &_shttp_connection_on_head_writed,
                                  &_shttp_connection_on_data_writed);

  http_parser_pause(&conn->parser, 1);
  return 0;
}

static inline void _relocate_buffer(shttp_connection_internal_t *conn, ptrdiff_t offset) {
  int        i;
  conn_incomming(conn).headers_buf.str += offset;
  conn_incomming(conn).url.str += offset;
  for(i =0; i <= conn_incomming(conn).headers.length; i ++) {
    conn_incomming(conn).headers.array[i].key.str += offset;
    conn_incomming(conn).headers.array[i].val.str += offset;
  }
}

static void _shttp_connection_on_alloc(uv_handle_t* req, size_t suggested_size, uv_buf_t* buf) {
  shttp_connection_internal_t *conn;
  size_t                      unused;
  void                        *p;

  conn = (shttp_connection_internal_t*)req->data;
  unused = conn->rd_buf.capacity - conn->rd_buf.end;

  if(0 < unused) {
    buf->base = conn->rd_buf.s + conn->rd_buf.end;
    buf->len = (unsigned long)unused;
    return;
  }

  p = spool_realloc(&conn->pool,
                    conn->rd_buf.s, conn->rd_buf.capacity + 2 * 1024);
  if(nil != p) {
    if(p != conn->rd_buf.s && shttp_headers_is_reading(conn)) {
      _relocate_buffer(conn, ((char*)p) - conn->rd_buf.s);
    }
    conn->rd_buf.s = (char*)p;
    conn->rd_buf.capacity += (2 * 1024);
    buf->base = conn->rd_buf.s + conn->rd_buf.end;
    buf->len = 2 * 1024;
    return;
  }

  buf->len = 0;
  return;
}

static void _shttp_connection_on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) {
  shttp_connection_internal_t *conn;

  conn = (shttp_connection_internal_t*)tcp->data;
  if (nread <= 0) {
    if (0 != nread && UV_EOF != nread) {
      ERR("read: %s", uv_strerror((int)nread));
    }
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return;
  }

  if(shttp_message_request_parse_none == conn->status) {
    _shttp_connection_parse_request_first(conn, buf->base, nread, nread);
  } else {
    _shttp_connection_parse_request(conn, buf->base, nread, nread);
  }
}

void _shttp_connection_on_connect(uv_stream_t* server_handle, int status) {
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
  // init pool and rd_buf
  spool_init(&conn->pool, (char*)conn->arena_base, conn->arena_capacity);
  conn->rd_buf.s = (char*)spool_malloc(&conn->pool, spool_excepted(2*1024));
  conn->rd_buf.start = 0;
  conn->rd_buf.end = 0;
  conn->rd_buf.capacity = spool_excepted(2*1024);

  // init uv_handle
  uv_tcp_init(http->uv_loop, &conn->uv_handle);
  conn->uv_handle.data = conn;

  // init http parser
  conn->status = shttp_message_request_parse_none;
  http_parser_init(&conn->parser, HTTP_REQUEST);
  conn->parser.data = conn;

  _shttp_connection_assert(http, conn);
  rc = uv_accept(server_handle, (uv_stream_t*)&conn->uv_handle);
  if(rc) {
    ERR("accept: %s\n", uv_strerror(rc));
    uv_close((uv_handle_t*)&conn->uv_handle, &_shttp_connection_on_disconnect);
    return ;
  }

  if(nil != conn->callbacks->on_connected) {
    if(0 != conn->callbacks->on_connected(&conn_external(conn))) {
      ERR("callbacks: on_connected is failed.");
      uv_close((uv_handle_t*)&conn->uv_handle, &_shttp_connection_on_disconnect);
      return ;
    }
  }

  TAILQ_REMOVE(&http->free_connections, conn, next);
  TAILQ_INSERT_TAIL(&http->connections, conn, next);

  rc = uv_read_start((uv_stream_t*)&conn->uv_handle,
                     _shttp_connection_on_alloc,
                     _shttp_connection_on_read);

  if(rc) {
    ERR("accept: %s", uv_strerror(rc));
    uv_close((uv_handle_t*)&conn->uv_handle, &_shttp_connection_on_disconnect);
  }
}

DLL_VARIABLE void shttp_connection_pool_free (shttp_connection_t *external, void *data) {
  spool_free(external->pool, data);
}

DLL_VARIABLE void shttp_connection_c_free (shttp_connection_t *conn, void *data) {
  sl_free(data);
}

#ifdef __cplusplus
};
#endif