
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

static void _shttp_on_disconnect(uv_handle_t* handle) {
  shttp_connection_internal_t *conn;
  shttp_t            *http;
  conn = (shttp_connection_internal_t *)handle->data;
  http = conn->inner.http;

  TAILQ_REMOVE(&http->connections, conn, next);
  TAILQ_INSERT_TAIL(&http->free_connections, conn, next);
}

static void _shttp_on_connection_alloc(uv_handle_t* req, size_t suggested_size, uv_buf_t* buf) {
  shttp_connection_internal_t *conn = (shttp_connection_internal_t*)req->data;
  if(conn->incomming.start == conn->incomming.end) {
    conn->incomming.start =0;
    conn->incomming.end = 0;
  }
  buf->base = conn->incomming.base + conn->incomming.end;
  buf->len = conn->incomming.capacity - conn->incomming.end;
}

static void _shttp_on_connection_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) {
  size_t                      parsed;
  shttp_connection_internal_t *conn;

  conn = (shttp_connection_internal_t*)tcp->data;
  if (nread <= 0) {
    // No Data Read... Handle Error case
    if (0 != nread && UV_EOF != nread) {
      ERR("read: %s\n", uv_strerror(nread));
    }
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_on_disconnect);
    return;
  }

  parsed = http_parser_execute(&conn->incomming.parser, &conn->inner.http->parser_settings, buf->base, nread);
  if (nread != parsed) {
    if(HPE_OK != HTTP_PARSER_ERRNO(&conn->incomming.parser)) {
      ERR("parse error:%s", http_errno_name(HTTP_PARSER_ERRNO(&conn->incomming.parser)));
    } else {
      ERR("parse error: parsed is 0.");
    }
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_on_disconnect);
    return;
  }

#define adjust_buffer_offset1(reseved_size)                                                                              \
    assert(conn->incomming.stack[0].str >= (conn->incomming.base + conn->incomming.start));                  \
    assert((conn->incomming.stack[0].str + (reseved_size)) ==                                              \
      (conn->incomming.base + conn->incomming.end + nread));                                              \
    conn->incomming.start = conn->incomming.stack[0].str - conn->incomming.base;                             \
    conn->incomming.end = conn->incomming.start + (reseved_size);                                         \
                                                                                                        \
    if(conn->incomming.end != conn->incomming.capacity) {                                                 \
      if(shttp_cacheline_size < (conn->incomming.end - conn->incomming.start)) {                          \
        return;                                                                                         \
      }                                                                                                 \
    } else if(0 == conn->incomming.start) {                                                              \
      ERR("parse error: head line is too large.");                                                      \
      uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_on_disconnect);                                 \
      return;                                                                                           \
    }                                                                                                   \
    memmove(conn->incomming.base,                                                                        \
      conn->incomming.base + conn->incomming.start,                                                       \
      conn->incomming.end - conn->incomming.start);                                                       \
                                                                                                        \
    conn->incomming.end -= conn->incomming.start

#define adjust_buffer_offset2() conn->incomming.start = 0

  switch(conn->incomming.status) {
  case shttp_message_begin:
    conn->incomming.start = 0;
    conn->incomming.end = 0;
    return;
  case shttp_message_url:
  case shttp_message_field:
    adjust_buffer_offset1(conn->incomming.stack[0].len);
    conn->incomming.stack[0].str -= conn->incomming.start;
    adjust_buffer_offset2();
    return;
  case shttp_message_value:
    adjust_buffer_offset1((conn->incomming.stack[0].len+conn->incomming.stack[1].len));
    conn->incomming.stack[0].str -= conn->incomming.start;
    conn->incomming.stack[1].str -= conn->incomming.start;
    adjust_buffer_offset2();
    return;
  case shttp_message_body:
    conn->incomming.start = 0;
    conn->incomming.end = 0;
    return;
  case shttp_message_end:
    conn->incomming.start = 0;
    conn->incomming.end = 0;
    return;
  default:
    ERR("parse error: status is unknown(%d).", conn->incomming.status);
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_on_disconnect);
    return;
  }
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
    goto failed;
  }

  conn = TAILQ_LAST(&http->free_connections, shttp_connections_s);

  uv_tcp_init(http->uv_loop, &conn->uv_handle);
  conn->uv_handle.data = conn;

  http_parser_init(&conn->incomming.parser, HTTP_REQUEST);
  conn->incomming.parser.data = &conn->incomming;


  assert(conn->incomming.callbacks == &conn->inner.http->settings.callbacks);
  assert(conn->incomming.conn == conn->inner.internal);
  assert(conn->inner.http == http);
  assert(conn->inner.internal == conn);
  assert(conn->incomming.capacity == conn->inner.http->settings.read_buffer_size);
  assert(conn->outgoing.capacity == conn->inner.http->settings.write_buffer_size);

  conn->incomming.start =0;
  conn->incomming.end = 0;
  conn->outgoing.len =0;

  rc = uv_accept(server_handle, (uv_stream_t*)&conn->uv_handle);
  if(rc) {
    ERR("accept: %s\n", uv_strerror(rc));
    return ;
  }
  TAILQ_REMOVE(&http->free_connections, conn, next);
  TAILQ_INSERT_TAIL(&http->connections, conn, next);

  rc = uv_read_start((uv_stream_t*)&conn->uv_handle,
                     _shttp_on_connection_alloc,
                     _shttp_on_connection_read);

  if(rc) {
    ERR("accept: %s", uv_strerror(rc));
    uv_close((uv_handle_t*)&conn->uv_handle, &_shttp_on_disconnect);
    return ;
  }

  return ;
failed:
  _shttp_on_null_connect(server_handle, status);
  return ;
}

#ifdef __cplusplus
};
#endif