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
  shttp_connection_internal_t *inner;
  uv_close_cb on_disconnect;


  on_disconnect = (uv_close_cb)req->data;
  inner = (shttp_connection_internal_t*)req->handle->data;
  
  assert(0 == inner->outgoing.head_write_buffers.length);
  inner->outgoing.body_write_buffers.length = 0;
  _shttp_response_call_hooks_after_writed(inner, &inner->outgoing);
  if (status < 0) {
    ERR("write: %s", uv_strerror(status));
  }
  uv_close((uv_handle_t*)req->handle, on_disconnect);
}

void _shttp_response_send_error_message(shttp_connection_internal_t *conn,
                                     void (*on_disconnect)(uv_handle_t* handle),
                                     const char* message_str,
                                     size_t message_len) {
  int rc;
  
  conn->outgoing.body_write_buffers.array[0].base = message_str;
  conn->outgoing.body_write_buffers.array[0].len = message_len;
  conn->outgoing.body_write_buffers.length = 1;

  conn->outgoing.write_req.data = on_disconnect;
  rc = uv_write(&conn->outgoing.write_req,
      (uv_stream_t*)&conn->uv_handle,
      &conn->outgoing.body_write_buffers.array[0],
      (unsigned long)conn->outgoing.body_write_buffers.length,
      _on_error_message_writed);
  if(0 != rc) {
    ERR("write: %s", uv_strerror(rc));
    uv_close((uv_handle_t*) &conn->uv_handle, on_disconnect);
    return;
  }
}


void _shttp_response_send_error_message_format(shttp_connection_internal_t *conn,
                                        uv_close_cb on_disconnect,
                                        const char  *fmt,
                                        ...) {
  int                                  res;
  va_list                              args;
  char                                 *buf;
  size_t                               buf_len;
  
  va_start(args, fmt);
  buf_len = strlen(fmt) + 128;
  buf = (char*)sl_malloc(buf_len);

  res = vsnprintf(buf, buf_len, fmt, args);
  if(-1 == res) {
    ERR("write: format message failed, %s", strerror(errno));
    uv_close((uv_handle_t*) &conn->uv_handle, on_disconnect);
    sl_free(buf);
    va_end(args);
    return;
  }

  assert(0 == conn->outgoing.call_after_writed.length);
  assert(0 == conn->outgoing.head_write_buffers.length);
  assert(0 == conn->outgoing.body_write_buffers.length);

  conn->outgoing.body_write_buffers.array[0].base = buf;
  conn->outgoing.body_write_buffers.array[0].len = res;
  conn->outgoing.body_write_buffers.length = 1;

  conn->outgoing.call_after_writed.array[0].cb = &shttp_response_c_free;
  conn->outgoing.call_after_writed.array[0].data = buf;
  conn->outgoing.call_after_writed.length = 1;

  conn->outgoing.write_req.data = on_disconnect;
  res = uv_write(&conn->outgoing.write_req,
      (uv_stream_t*)&conn->uv_handle,
      &conn->outgoing.body_write_buffers.array[0],
      (unsigned long)conn->outgoing.body_write_buffers.length,
      _on_error_message_writed);
  if(0 != res) {
    ERR("write: %s", uv_strerror(res));
    uv_close((uv_handle_t*) &conn->uv_handle, on_disconnect);
    va_end(args);
    return;
  }

  va_end(args);
}

#ifdef __cplusplus
}
#endif