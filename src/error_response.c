#include "squirrel_config.h"

#include <assert.h>

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * HTTP Utility Functions
 ****************************************************************************/

static void _on_error_message_writed(uv_write_t* req, int status) {
  shttp_connection_internal_t *conn;
  uv_close_cb on_disconnect;


  on_disconnect = (uv_close_cb)req->data;
  conn = (shttp_connection_internal_t*)req->handle->data;

  shttp_assert(0 == conn_outgoing(conn).head_write_buffers.length);
  conn_outgoing(conn).body_write_buffers.length = 0;
  _shttp_response_call_hooks_after_writed(conn);
  if (status < 0) {
    ERR("write: %s", uv_strerror(status));
  }
  uv_close((uv_handle_t*)req->handle, on_disconnect);
}

void _shttp_response_send_error_message(shttp_connection_internal_t *conn,
                                        const char* message_str,
                                        size_t message_len) {
  int rc;

  _shttp_response_on_completed(conn, -1);

  conn_outgoing(conn).body_write_buffers.array[0].base = (char*)message_str;
  conn_outgoing(conn).body_write_buffers.array[0].len = (ULONG)message_len;
  conn_outgoing(conn).body_write_buffers.length = 1;

  conn_outgoing(conn).write_req.data = &_shttp_connection_on_disconnect;
  rc = uv_write(&conn_outgoing(conn).write_req,
                (uv_stream_t*)&conn->uv_handle,
                &conn_outgoing(conn).body_write_buffers.array[0],
                (unsigned long)conn_outgoing(conn).body_write_buffers.length,
                _on_error_message_writed);
  if(0 != rc) {
    ERR("write: %s", uv_strerror(rc));
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    return;
  }
}


void _shttp_response_send_error_message_format(shttp_connection_internal_t *conn,
    const char  *fmt,
    ...) {
  int                                  res;
  va_list                              args;
  char                                 *buf;
  size_t                               buf_len;

  _shttp_response_on_completed(conn, -1);

  va_start(args, fmt);
  buf_len = strlen(fmt) + 128;
  buf = (char*)sl_malloc(buf_len);

  res = vsnprintf(buf, buf_len, fmt, args);
  if(-1 == res) {
    ERR("write: format message failed, %s", strerror(errno));
    uv_close((uv_handle_t*) &conn->uv_handle,
             &_shttp_connection_on_disconnect);
    sl_free(buf);
    va_end(args);
    return;
  }

  shttp_assert(0 == conn_outgoing(conn).call_after_completed.length);
  shttp_assert(0 == conn_outgoing(conn).call_after_data_writed.length);
  shttp_assert(0 == conn_outgoing(conn).head_write_buffers.length);
  shttp_assert(0 == conn_outgoing(conn).body_write_buffers.length);

  conn_outgoing(conn).body_write_buffers.array[0].base = buf;
  conn_outgoing(conn).body_write_buffers.array[0].len = res;
  conn_outgoing(conn).body_write_buffers.length = 1;

  conn_outgoing(conn).call_after_data_writed.array[0].cb = &shttp_response_c_free;
  conn_outgoing(conn).call_after_data_writed.array[0].data = buf;
  conn_outgoing(conn).call_after_data_writed.length = 1;

  conn_outgoing(conn).write_req.data = &_shttp_connection_on_disconnect;
  res = uv_write(&conn_outgoing(conn).write_req,
                 (uv_stream_t*)&conn->uv_handle,
                 &conn_outgoing(conn).body_write_buffers.array[0],
                 (unsigned long)conn_outgoing(conn).body_write_buffers.length,
                 _on_error_message_writed);
  if(0 != res) {
    ERR("write: %s", uv_strerror(res));
    uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
    va_end(args);
    return;
  }

  va_end(args);
}

#ifdef __cplusplus
}
#endif