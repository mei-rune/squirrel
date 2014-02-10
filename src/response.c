#include "squirrel_config.h"
#include "response_helper.h"

#ifdef __cplusplus
extern "C" {
#endif

void _shttp_response_init(shttp_context_internal_t *ctx) {
  
  memset(&ctx_response(ctx), 0, sizeof(shttp_response_t));

  ctx_outgoing(ctx).write_req.data = nil;
  ctx_outgoing(ctx).head_write_buffers.length = 0;
  ctx_outgoing(ctx).body_write_buffers.length = 0;
  ctx_outgoing(ctx).call_after_completed.length = 0;
  ctx_outgoing(ctx).call_after_data_writed.length = 0;
  ctx_outgoing(ctx).failed_code = 0;
  ctx_outgoing(ctx).is_body_end = 0;


  if(0 != shttp_atomic_read32(&ctx_outgoing(ctx).is_async)) {
    abort();
  }
  if(0 != shttp_atomic_read32(&ctx_outgoing(ctx).is_writing)) {
    abort();
  }
  if(0 != shttp_atomic_read32(&ctx_outgoing(ctx).thread_id)) {
    abort();
  }
  ctx_outgoing(ctx).reserved1 = 0;

  ctx_response(ctx).headers.array = ctx_outgoing(ctx).headers.array;
  ctx_response(ctx).status_code = SHTTP_STATUS_OK;
  ctx_response(ctx).chunked = SHTTP_THUNKED_NONE;
  ctx_response(ctx).close_connection = SHTTP_CLOSE_CONNECTION_NONE;
}


void _shttp_response_destory(shttp_context_internal_t *ctx) {
}

DLL_VARIABLE void shttp_response_pool_free (shttp_context_t *ctx, void *data) {
  spool_free(ctx->pool, data);
}

DLL_VARIABLE void shttp_response_c_free (shttp_context_t *ctx, void *data) {
  sl_free(data);
}

DLL_VARIABLE shttp_res shttp_response_call_after_writed(shttp_context_t *external,
    shttp_context_write_cb cb, void *cb_data) {
  shttp_context_internal_t *ctx;

  ctx = to_internal(external);
  HEAD_AND_BODY_IS_NOT_END(ctx);
  if(nil == cb) {
    return SHTTP_RES_INVALID_PARAMETERS;
  }

  CALL_AFTER(ctx, data_writed, cb, cb_data);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_call_after_completed(shttp_context_t *external,
    shttp_context_write_cb cb, void *cb_data) {
  shttp_context_internal_t *ctx;

  ctx = to_internal(external);
  HEAD_AND_BODY_IS_NOT_END(ctx);
  if(nil == cb) {
    return SHTTP_RES_INVALID_PARAMETERS;
  }
  CALL_AFTER(ctx, completed, cb, cb_data);
  return SHTTP_RES_OK;
}


DLL_VARIABLE shttp_res shttp_response_set_async(shttp_context_t *external) {
  shttp_context_internal_t *ctx;

  ctx = to_internal(external);
  ctx->is_async = 1;

  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_start(shttp_context_t *external,
    uint16_t status, const char *content_type, size_t content_type_len) {
  shttp_context_internal_t *ctx;
  shttp_response_t *response;

  ctx = to_internal(external);
  response = external->response;

  HEAD_AND_BODY_IS_NOT_END(ctx);
  response->status_code = status;
  response->http_major = external->request->http_major;
  response->http_minor = external->request->http_minor;
  copy_content_type(ctx, response, content_type, content_type_len);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_set_chuncked(shttp_context_t *external) {
  shttp_context_internal_t *ctx;
  shttp_response_t *response;

  ctx = to_internal(external);
  response = external->response;

  HEAD_AND_BODY_IS_NOT_END(ctx);
  external->response->chunked = 1;
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_write_header(shttp_context_t *external,
    const char *key, size_t key_len, const char *value, size_t value_len) {
  size_t                                estimate_length, new_capacity;
  char                                 *p;
  shttp_context_internal_t             *ctx;
  shttp_response_t                     *response;

  ctx = to_internal(external);
  response = external->response;

  HEAD_AND_BODY_IS_NOT_END(ctx);

  do_predefine_headers(ctx, response, key, key_len, value, value_len);

  if(response->headers.length >= ctx_outgoing(ctx).headers.capacity) {
    return SHTTP_RES_HEAD_TOO_LARGE;
  }

  estimate_length = key_len + SHTTP_DELIMITER_LEN + value_len + SHTTP_CRLF_LEN;
  try_realloc_head_buffer(ctx, estimate_length, return_stmt);

mem_copy_begin:
  ctx_response_last_header(ctx).key.str = p;
  ctx_response_last_header(ctx).key.len = key_len;
  memcpy(p, key, key_len);
  p += key_len;
  head_buffer_append_delimiter(p);
  p += SHTTP_DELIMITER_LEN;

  ctx_response_last_header(ctx).val.str = p;
  ctx_response_last_header(ctx).val.len = value_len;
  memcpy(p, value, value_len);
  p += value_len;
  head_buffer_append_crlf(p);
  p += SHTTP_CRLF_LEN;

  sbuffer_length_add(ctx_outgoing(ctx).head_buffer,
                     p - (sbuffer_unused_addr(ctx_outgoing(ctx).head_buffer)));
  response->headers.length += 1;
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_vformat_header(shttp_context_t *external,
    const char *key,
    size_t      key_len,
    const char *fmt,
    va_list     args) {
  size_t                               estimate_length, new_capacity;
  int                                  res;
  char                                 *p;
  boolean                              tried;
  shttp_context_internal_t             *ctx;
  shttp_response_t                     *response;

  ctx = to_internal(external);
  response = external->response;
  
  HEAD_AND_BODY_IS_NOT_END(ctx);

  if(response->headers.length >= ctx_outgoing(ctx).headers.capacity) {
    return SHTTP_RES_HEAD_TOO_LARGE;
  }

  tried = false;
  estimate_length = key_len + SHTTP_DELIMITER_LEN + strlen(fmt) + 32 + SHTTP_CRLF_LEN;

retry:
  try_realloc_head_buffer(ctx, estimate_length, return_stmt);

mem_copy_begin:
  new_capacity = sbuffer_unused_size(ctx_outgoing(ctx).head_buffer);
  p = sbuffer_unused_addr(ctx_outgoing(ctx).head_buffer);
  res = vsnprintf(p + key_len + SHTTP_DELIMITER_LEN,
                  new_capacity - key_len - SHTTP_DELIMITER_LEN - SHTTP_CRLF_LEN, fmt, args);
  if(-1 == res) {
    if(tried) {
      return SHTTP_RES_PRINTF;
    }
    tried = true;
    estimate_length = key_len + SHTTP_DELIMITER_LEN + vscprintf(fmt, args) + SHTTP_CRLF_LEN;
    goto retry;
  }
  do_predefine_headers(ctx, response, key, key_len,
                       p + key_len + SHTTP_DELIMITER_LEN, res);

  ctx_response_last_header(ctx).key.str = p;
  ctx_response_last_header(ctx).key.len = key_len;
  memcpy(p, key, key_len);
  p += key_len;
  head_buffer_append_delimiter(p);
  p += SHTTP_DELIMITER_LEN;
  p += res;
  ctx_response_last_header(ctx).val.str = p;
  ctx_response_last_header(ctx).val.len = res;
  head_buffer_append_crlf(p);
  p += SHTTP_CRLF_LEN;

  sbuffer_length_add(ctx_outgoing(ctx).head_buffer,
                     p - sbuffer_unused_addr(ctx_outgoing(ctx).head_buffer) );
  response->headers.length += 1;

  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_format_header(shttp_context_t *external,
    const char *key,
    size_t     key_len,
    const char *fmt,
    ...) {
  int                                  rc;
  va_list                              args;
  va_start(args, fmt);
  rc = shttp_response_vformat_header(external, key, key_len, fmt, args);
  va_end(args);
  return rc;
}


DLL_VARIABLE shttp_res shttp_response_write_nocopy(shttp_context_t *external,
    const char *data, size_t length, shttp_context_write_cb cb, void *cb_data) {
  shttp_context_internal_t             *ctx;
  shttp_response_t                     *response;

  ctx = to_internal(external);
  response = external->response;
  
  if(SHTTP_RES_OK != ctx_outgoing(ctx).failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == ctx_outgoing(ctx).is_body_end) {
    shttp_assert(0 == ctx_outgoing(ctx).is_body_end);
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }

  ctx_outgoing(ctx).body_buffer.str = nil;
  WRITE_TO_UV_BUF(ctx, body_write_buffers, (char*)data, length);
  if(nil != cb) {
    CALL_AFTER(conn, data_writed, cb, cb_data);
  }
  response->content_length += length;
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_write(shttp_context_t *external,
    const char *data, size_t length) {
  shttp_context_internal_t             *ctx;
  shttp_response_t                     *response;

  ctx = to_internal(external);
  response = external->response;
      
  if(SHTTP_RES_OK != ctx_outgoing(ctx).failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == ctx_outgoing(ctx).is_body_end) {
    shttp_assert(0 == ctx_outgoing(ctx).is_body_end);
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }


  if(nil != ctx_outgoing(ctx).body_buffer.str &&
      ctx_outgoing(ctx).body_buffer.str == ctx_last_data_buffer(conn).base) {

    if(length <= sbuffer_unused_size(ctx_outgoing(ctx).body_buffer)) {
      goto mem_copy;
    }

    if(nil != (char*)ctx_response_try_realloc(ctx,
        ctx_outgoing(ctx).body_buffer.str,
        ctx_outgoing(ctx).body_buffer.capacity + length)) {
      ctx_outgoing(ctx).body_buffer.capacity += length;
      goto mem_copy;
    }
  }

  ctx_outgoing(ctx).body_buffer.capacity = spool_excepted(length);
  ctx_outgoing(ctx).body_buffer.str =
    (char*)ctx_response_malloc(ctx, ctx_outgoing(ctx).body_buffer.capacity);
  if(nil == ctx_outgoing(ctx).body_buffer.str) {
    return SHTTP_RES_MEMORY;
  }
  memcpy(ctx_outgoing(ctx).body_buffer.str, data, length);
  ctx_outgoing(ctx).body_buffer.len = length;

  WRITE_TO_UV_BUF(conn,
                  body_write_buffers,
                  ctx_outgoing(ctx).body_buffer.str,
                  length);
  CALL_AFTER(ctx, data_writed,
             &shttp_response_pool_free,
             ctx_outgoing(ctx).body_buffer.str);
  response->content_length += length;
  return SHTTP_RES_OK;

mem_copy:
  memcpy(sbuffer_unused_addr(ctx_outgoing(ctx).body_buffer), data, length);
  ctx_last_data_buffer(conn).len += (unsigned long)length;
  ctx_outgoing(ctx).body_buffer.len += length;
  response->content_length += length;
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_vformat(shttp_context_t *external,
    const char *fmt, va_list args) {
  size_t                                length;
  int                                   retries;
  shttp_context_internal_t             *ctx;
  shttp_response_t                     *response;

  ctx = to_internal(external);
  response = external->response;


  if(SHTTP_RES_OK != ctx_outgoing(ctx).failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == ctx_outgoing(ctx).is_body_end) {
    shttp_assert(0 == ctx_outgoing(ctx).is_body_end);
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }

#define ctx_data_buffer(conn) ctx_outgoing(ctx).body_write_buffers
#define last_ctx_data_buffer(conn) ctx_data_buffer(conn).array[ctx_data_buffer(conn).length - 1]

  length = strlen(fmt) + 64;
  retries = 0;

try_malloc:
  if(nil != ctx_outgoing(ctx).body_buffer.str &&
      ctx_outgoing(ctx).body_buffer.str == last_ctx_data_buffer(conn).base) {

    if(length <= sbuffer_unused_size(ctx_outgoing(ctx).body_buffer)) {
      goto mem_copy;
    }
    length = shttp_align(sl_max(length, 256), 256);
    if(nil != (char*)ctx_response_try_realloc(ctx,
        ctx_outgoing(ctx).body_buffer.str,
        ctx_outgoing(ctx).body_buffer.capacity + length)) {
      ctx_outgoing(ctx).body_buffer.capacity += length;
      goto mem_copy;
    }
  }

  ctx_outgoing(ctx).body_buffer.capacity = spool_excepted(sl_max(length, 256));
  ctx_outgoing(ctx).body_buffer.str = (char*)ctx_response_malloc(ctx,
                                      ctx_outgoing(ctx).body_buffer.capacity);
  if(nil == ctx_outgoing(ctx).body_buffer.str) {
    return SHTTP_RES_MEMORY;
  }
  length = vsnprintf(ctx_outgoing(ctx).body_buffer.str,
                     ctx_outgoing(ctx).body_buffer.capacity, fmt, args);
  if(-1 == length) {
    if(0 != retries) {
      return SHTTP_RES_PRINTF;
    }
    length = spool_excepted(vscprintf(fmt, args) + 64);
    retries ++;
    goto try_malloc;
  }
  ctx_outgoing(ctx).body_buffer.len = length;

  WRITE_TO_UV_BUF(conn,
                  body_write_buffers,
                  ctx_outgoing(ctx).body_buffer.str,
                  length);
  CALL_AFTER(ctx,
             data_writed,
             shttp_response_pool_free,
             ctx_outgoing(ctx).body_buffer.str);
  response->content_length += length;
  return SHTTP_RES_OK;

mem_copy:
  length = vsnprintf(sbuffer_unused_addr(ctx_outgoing(ctx).body_buffer),
                     sbuffer_unused_size(ctx_outgoing(ctx).body_buffer), fmt, args);
  if(-1 == length) {
    if(0 != retries) {
      return SHTTP_RES_PRINTF;
    }
    length = spool_excepted(vscprintf(fmt, args) + 64);
    retries ++;
    goto try_malloc;
  }
  last_ctx_data_buffer(ctx).len += (unsigned long)length;
  ctx_outgoing(ctx).body_buffer.len += length;
  response->content_length += length;
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_format(shttp_context_t *external,
    const char *fmt, ...) {
  va_list                              args;
  shttp_res                            rc;

  va_start(args, fmt);
  rc = shttp_response_vformat(external, fmt, args);
  va_end(args);

  return rc;
}

DLL_VARIABLE shttp_res shttp_response_end(shttp_context_t *external) {
  shttp_context_internal_t             *ctx;
  shttp_response_t                     *response;

  ctx = to_internal(external);
  response = external->response;

  HEAD_AND_BODY_IS_NOT_END(ctx);

  ctx_outgoing(ctx).is_body_end = 1;
  return SHTTP_RES_OK;
}

void _shttp_response_on_completed(shttp_connection_t *conn, shttp_context_t *external, int status) {
  shttp_context_internal_t             *ctx;
  shttp_response_t                     *response;

  ctx = to_internal(external);
  response = external->response;

  shttp_assert((0 == status)? true : (SHTTP_RES_OK != ctx_outgoing(ctx).failed_code));
  ctx_outgoing(ctx).head_write_buffers.length = 0;
  ctx_outgoing(ctx).body_write_buffers.length = 0;
  _shttp_response_call_hooks_after_writed(conn, ctx);
  _shttp_response_call_hooks_after_completed(conn, ctx);

  _shttp_response_assert_after_response_end(conn);

  if(nil != ctx_outgoing(ctx).content_type.str) {
    ctx_response_free(ctx, ctx_outgoing(ctx).content_type.str);
    ctx_outgoing(ctx).content_type.len = 0;
    ctx_outgoing(ctx).content_type.str = nil;
  }

  shttp_atomic_set32(&ctx_outgoing(ctx).is_async, 0);
  shttp_atomic_set32(&ctx_outgoing(ctx).is_writing, 0);
  shttp_atomic_set32(&ctx_outgoing(ctx).thread_id, 0);
}

static inline shttp_res _shttp_response_send_predefine_headers(shttp_context_internal_t *ctx) {
  cstring_t          *status_code;
  int                 res;
  shttp_res           rc;
  sbuf_t              buf;
  shttp_response_t   *response;

  response = &ctx->outgoing.response;

  status_code = shttp_status_code_text(response->status_code);
  if(nil == status_code) {
    return SHTTP_RES_ERROR_STATUS_CODE;
  }

#define _shttp_header_copy(ctx, buf, buf_len)                 \
  rc = _shttp_response_write_header_copy(ctx, buf, buf_len);  \
  if(SHTTP_RES_OK != rc) {                                     \
    return rc;                                                 \
  }

  //  Connection header
  _shttp_header_copy(ctx,
                     "Connection" SHTTP_DELIMITER_STR,
                     10 + SHTTP_DELIMITER_LEN);

  switch(response->close_connection) {
  case SHTTP_CLOSE_CONNECTION_NONE:
    if(0 == http_should_keep_alive(&ctx->parser)) {
      _shttp_header_copy(ctx,
                         SHTTP_CONNECTION_CLOSE_STR SHTTP_CRLF_STR,
                         SHTTP_CONNECTION_CLOSE_LEN + SHTTP_CRLF_LEN);
    } else {
      _shttp_header_copy(ctx,
                         SHTTP_CONNECTION_KEEP_ALIVE_STR SHTTP_CRLF_STR,
                         SHTTP_CONNECTION_KEEP_ALIVE_LEN + SHTTP_CRLF_LEN);
    }
    break;
  case SHTTP_CLOSE_CONNECTION_TRUE:
    _shttp_header_copy(ctx,
                       SHTTP_CONNECTION_KEEP_ALIVE_STR SHTTP_CRLF_STR,
                       SHTTP_CONNECTION_KEEP_ALIVE_LEN + SHTTP_CRLF_LEN);
    break;
  default:
    _shttp_header_copy(ctx,
                       SHTTP_CONNECTION_CLOSE_STR SHTTP_CRLF_STR,
                       SHTTP_CONNECTION_CLOSE_LEN + SHTTP_CRLF_LEN);
    break;
  }

  // Content-Type Header
  _shttp_header_copy(ctx,
                     "Content-Type" SHTTP_DELIMITER_STR,
                     12 + SHTTP_DELIMITER_LEN);
  _shttp_header_copy(ctx,
                     response->content_type.str,
                     response->content_type.len);
  _shttp_header_copy(ctx, SHTTP_CRLF_STR, SHTTP_CRLF_LEN);

  // Transfer-Encoding AND Content-Length Header
  if(SHTTP_THUNKED_TRUE == response->chunked) {
    _shttp_header_copy(ctx,
                       "Transfer-Encoding" SHTTP_DELIMITER_STR "chunked" SHTTP_CRLF_STR,
                       17 + SHTTP_DELIMITER_LEN + 7 + SHTTP_CRLF_LEN);
  } else if(((size_t)-1) != response->content_length) { // Content-Length Header
    rc = _shttp_response_write_header_format(ctx, 28,
         "Content-Length" SHTTP_DELIMITER_STR "%d" SHTTP_CRLF_STR,
         response->content_length);
    if(SHTTP_RES_OK != rc) {
      return rc;
    }
  }
  if(0 == ctx_outgoing(ctx).head_write_buffers.length) {
    ctx_outgoing(ctx).head_write_buffers.length = 1;
  }

  _shttp_header_copy(ctx, SHTTP_CRLF_STR, SHTTP_CRLF_LEN);
  WRITE_TO_UV_BUF(ctx, head_write_buffers,
                  ctx_outgoing(ctx).head_buffer.str,
                  sbuffer_length(ctx_outgoing(ctx).head_buffer));
  sbuffer_zero(ctx_outgoing(ctx).head_buffer);


  res = spool_prepare_alloc(ctx->external.pool, &buf);
  if(SHTTP_RES_OK != res) {
    return res;
  }

  res = snprintf(buf.str, buf.len, "HTTP/%d.%d %d ",
                 response->http_major,
                 response->http_minor,
                 response->status_code);
  if(-1 == res) {
    goto failed;
  }

  if((res + status_code->len + SHTTP_CRLF_LEN) > buf.len) {
    goto failed;
  }
  memcpy(buf.str + res, status_code->str, status_code->len);
  head_buffer_append_crlf(buf.str + res + status_code->len);

  ctx_outgoing(ctx).head_write_buffers.array[0].base = buf.str;
  ctx_outgoing(ctx).head_write_buffers.array[0].len = (ULONG)(res + status_code->len + SHTTP_CRLF_LEN);
  if(0 == ctx_outgoing(ctx).head_write_buffers.length) {
    ctx_outgoing(ctx).head_write_buffers.length = 1;
  }

  spool_commit_alloc(&(ctx->pool), res + status_code->len + SHTTP_CRLF_LEN);

  call_after(ctx, completed, &shttp_response_pool_free, buf.str, goto_stmt);
  return SHTTP_RES_OK;
failed:
  spool_rollback_alloc(&(ctx->pool), &buf);
  return SHTTP_RES_MEMORY;
}

void _shttp_response_send_immediate(shttp_connection_t *external_conn, shttp_context_t *external) {
  int                                   rc = SHTTP_RES_OK;
  shttp_connection_internal_t          *conn;
  shttp_context_internal_t             *ctx;
  shttp_response_t                     *response;

  conn = (shttp_connection_internal_t*)external_conn->internal;
  ctx = to_internal(external);
  response = external->response;

  if(SHTTP_RES_OK != ctx_outgoing(ctx).failed_code) {
    _shttp_response_send_error_message_format(conn,
        INTERNAL_ERROR_FROMAT, shttp_strerr(ctx_outgoing(ctx).failed_code));
    return;
  }

  if(0 == ctx_outgoing(ctx).is_body_end &&
      0 == response->chunked) {
    ERR("callback: chunked must is true while body is not completed.");
    ctx_outgoing(ctx).failed_code = SHTTP_RES_BODY_NOT_COMPLETE;
    _shttp_response_send_error_message(conn,
                                       BODY_NOT_COMPLETE,
                                       strlen(BODY_NOT_COMPLETE));
    return;
  }

  if(1 != ctx_outgoing(ctx).head_writed) {
    shttp_assert(0 == ctx_outgoing(ctx).is_body_end);
    rc = _shttp_response_send_predefine_headers(ctx);
    if(SHTTP_RES_OK != rc) {
      ERR("callback: write predefine header failed.");
      goto failed;
    }
    ctx_outgoing(ctx).head_writed = 1;
  }

  if(0 == ctx_outgoing(ctx).head_write_buffers.length) {
    if(0 == ctx_outgoing(ctx).body_write_buffers.length) {
      if(0 != ctx_outgoing(ctx).is_body_end) {
        _shttp_connection_restart_read_request(conn);
        return;
      }
      ERR("callback: write buffers is empty.");
      rc = SHTTP_RES_RESPONSE_BODY_IS_EMPTY;
      goto failed;
    }
    rc = uv_write(&ctx_outgoing(ctx).write_req,
                  (uv_stream_t*)&conn->uv_handle,
                  &ctx_outgoing(ctx).body_write_buffers.array[0],
                  (unsigned long)ctx_outgoing(ctx).body_write_buffers.length,
                  &_shttp_connection_on_data_writed);
  } else {
    rc = uv_write(&ctx_outgoing(ctx).write_req,
                  (uv_stream_t*)&conn->uv_handle,
                  &ctx_outgoing(ctx).head_write_buffers.array[0],
                  (unsigned long)ctx_outgoing(ctx).head_write_buffers.length,
                  &_shttp_connection_on_head_writed);
  }
  if(0 == rc) {
    return;
  }
  ERR("write: %s", uv_strerror(rc));
failed:
  ctx_outgoing(ctx).failed_code = rc;
  _shttp_response_on_completed(conn, -1);
  uv_close((uv_handle_t*) &conn->uv_handle, &_shttp_connection_on_disconnect);
  return;
}

#ifdef __cplusplus
};
#endif