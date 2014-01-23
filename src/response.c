#include "squirrel_config.h"

#include <assert.h>

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WRITE_TO_UV_BUF(conn, write_buffers, buf, buf_len)                                                             \
    if(conn_outgoing(conn).write_buffers.capacity <= conn_outgoing(conn).write_buffers.length) {                       \
      return SHTTP_RES_INSUFFICIENT_WRITE_BUFFER;                                                                      \
    }                                                                                                                  \
    conn_outgoing(conn).write_buffers.array[conn_outgoing(conn).write_buffers.length].base = (buf);                    \
    conn_outgoing(conn).write_buffers.array[conn_outgoing(conn).write_buffers.length].len = (ULONG)(buf_len);          \
    conn_outgoing(conn).write_buffers.length ++


#define CALL_AFTER(conn, action, func, func_data) if(conn_outgoing(conn).call_after_##action.capacity <=               \
                        conn_outgoing(conn).call_after_##action.length) {                                              \
      return SHTTP_RES_INSUFFICIENT_WRITE_CB_BUFFER;                                                                   \
    }                                                                                                                  \
    conn_outgoing(conn).call_after_##action.array[conn_outgoing(conn).call_after_##action.length].cb = (func);         \
    conn_outgoing(conn).call_after_##action.array[conn_outgoing(conn).call_after_##action.length].data = (func_data)  ;\
    conn_outgoing(conn).call_after_##action.length ++

#define conn_response_malloc(conn, size) spool_malloc(&(conn)->pool, (size))
#define conn_response_realloc(inner, p, size) spool_realloc(&(conn)->pool, (p), (size))
#define conn_response_free(inner, p) spool_free(&(conn)->pool, (p))
#define conn_response_pool(conn) &(conn)->pool

#define copy_content_type(conn, txt, txt_len)                                       \
  if(txt_len > conn_outgoing(conn).content_type.len) {                              \
    if(nil == conn_outgoing(conn).content_type.len) {                               \
      conn_outgoing(conn).content_type.len = sl_max(128, txt_len);                  \
      conn_outgoing(conn).content_type.str = (char*)conn_response_malloc(conn,      \
                                                     sl_max(128, txt_len));         \
      if(nil == conn_outgoing(conn).content_type.str) {                             \
        return SHTTP_RES_MEMORY;                                                    \
      }                                                                             \
    } else {                                                                        \
      conn_outgoing(conn).content_type.len = txt_len;                               \
      conn_outgoing(conn).content_type.str = (char*)conn_response_realloc(conn,     \
        conn_outgoing(conn).content_type.str, txt_len);                             \
      if(nil == conn_outgoing(conn).content_type.str) {                             \
        return SHTTP_RES_MEMORY;                                                    \
      }                                                                             \
    }                                                                               \
  }                                                                                 \
  conn_response(conn).content_type.str = conn_outgoing(conn).content_type.str;      \
  conn_response(conn).content_type.len = txt_len;                                   \
  memcpy(conn_outgoing(conn).content_type.str, txt, txt_len)



DLL_VARIABLE shttp_res shttp_response_call_after_writed(shttp_connection_t *external,
    shttp_write_cb cb, void *cb_data) {
  shttp_connection_internal_t          *conn;
  conn = (shttp_connection_internal_t*)external->internal;

  if(SHTTP_RES_OK != conn_outgoing(conn).failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == conn_outgoing(conn).is_body_end) {
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }
  if(nil == cb) {
    return SHTTP_RES_INVALID_PARAMETERS;
  }
  CALL_AFTER(conn, data_writed, cb, cb_data);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_call_after_completed(shttp_connection_t *external,
    shttp_write_cb cb, void *cb_data) {
  shttp_connection_internal_t          *conn;
  conn = (shttp_connection_internal_t*)external->internal;

  if(SHTTP_RES_OK != conn_outgoing(conn).failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == conn_response(conn).head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  if(1 == conn_outgoing(conn).is_body_end) {
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }
  if(nil == cb) {
    return SHTTP_RES_INVALID_PARAMETERS;
  }
  CALL_AFTER(conn, completed, cb, cb_data);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_start(shttp_connection_t *external,
    uint16_t status,
    const char *content_type,
    size_t content_type_len) {
  shttp_connection_internal_t          *conn;

  conn = (shttp_connection_internal_t*)external->internal;
  if(1 == conn_response(conn).head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  assert(0 == conn_outgoing(conn).is_body_end);

  conn_response(conn).status_code = status;
  conn_response(conn).http_major = conn_request(conn).http_major;
  conn_response(conn).http_minor = conn_request(conn).http_minor;
  copy_content_type(conn, content_type, content_type_len);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_set_chuncked(shttp_connection_t *external) {
  shttp_connection_internal_t          *conn;
  conn = (shttp_connection_internal_t*)external->internal;
  if(1 == conn_response(conn).head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  assert(0 == conn_outgoing(conn).is_body_end);
  conn_response(conn).chunked = 1;
  return SHTTP_RES_OK;
}

DLL_VARIABLE void shttp_response_pool_free (shttp_connection_t *external, void *data) {
  spool_free(external->pool, data);
}

DLL_VARIABLE void shttp_response_c_free (shttp_connection_t *conn, void *data) {
  sl_free(data);
}


#define do_predefine_headers(conn, key, key_len, value, value_len)                         \
  if(12 == key_len && 0 == strncmp("Content-Type", key, key_len)) {                        \
    copy_content_type(conn, value, value_len);                                             \
    return SHTTP_RES_OK;                                                                   \
  } else if(17 == key_len && 0 == strncmp("Transfer-Encoding", key, key_len)) {            \
    if(7 == value_len && 0 == strncasecmp("chunked", value, value_len)) {                  \
      if(SHTTP_THUNKED_NONE == conn_response(conn).chunked ||                              \
        SHTTP_THUNKED_TRUE ==  conn_response(conn).chunked) {                              \
        conn_response(conn).chunked = SHTTP_THUNKED_TRUE;                                  \
        return SHTTP_RES_OK;                                                               \
      }                                                                                    \
    }                                                                                      \
    return SHTTP_RES_HEAD_TRANSFER_ENCODING;                                               \
  } else if(10 == key_len && 0 == strncmp("Connection", key, key_len)) {                   \
    if(5 == value_len && 0 == strncasecmp("close", value, value_len)) {                    \
       if(SHTTP_CLOSE_CONNECTION_NONE == conn_response(conn).close_connection ||           \
        SHTTP_CLOSE_CONNECTION_TRUE ==  conn_response(conn).close_connection) {            \
        conn_response(conn).close_connection = SHTTP_CLOSE_CONNECTION_TRUE;                \
        return SHTTP_RES_OK;                                                               \
      }                                                                                    \
    } else if(10 == value_len && 0 == strncasecmp("keep-alive", value, value_len)) {       \
      if(SHTTP_CLOSE_CONNECTION_NONE == conn_response(conn).close_connection ||            \
        SHTTP_CLOSE_CONNECTION_FALSE ==  conn_response(conn).close_connection) {           \
        conn_response(conn).close_connection = SHTTP_CLOSE_CONNECTION_FALSE;               \
        return SHTTP_RES_OK;                                                               \
      }                                                                                    \
    }                                                                                      \
    return SHTTP_RES_HEAD_CONNECTION;                                                      \
  }


#define try_realloc_head_buffer(conn, estimate_length, return_stmt)                        \
  if(nil != conn_outgoing(conn).head_buffer.str) {                                         \
    new_capacity = sbuffer_unused_size(conn_outgoing(conn).head_buffer);                   \
    if(estimate_length < new_capacity) {                                                   \
      p = sbuffer_unused_addr(conn_outgoing(conn).head_buffer);                            \
      goto mem_copy_begin;                                                                 \
    }                                                                                      \
                                                                                           \
    new_capacity = conn_outgoing(conn).head_buffer.capacity +                              \
      shttp_align(sl_max(estimate_length, 256), shttp_cacheline_size);                     \
                                                                                           \
    if(nil != spool_try_realloc(pool,                                                      \
      conn_outgoing(conn).head_buffer.str,                                                 \
      new_capacity)) {                                                                     \
      conn_outgoing(conn).head_buffer.capacity = new_capacity;                             \
      p = sbuffer_unused_addr(conn_outgoing(conn).head_buffer);                            \
      goto mem_copy_begin;                                                                 \
    }                                                                                      \
                                                                                           \
    if(0 == conn_outgoing(conn).head_write_buffers.length) {                               \
       /* this is for first line of response.*/                                            \
      conn_outgoing(conn).head_write_buffers.length = 1;                                   \
    }                                                                                      \
    if(0 != sbuffer_length(conn_outgoing(conn).head_buffer)) {                             \
      WRITE_TO_UV_BUF(conn, head_write_buffers, conn_outgoing(conn).head_buffer.str,       \
        sbuffer_length(conn_outgoing(conn).head_buffer));                                  \
    }                                                                                      \
  }                                                                                        \
                                                                                           \
  new_capacity = spool_excepted(sl_max(estimate_length, 256));                             \
  p = (char*)spool_malloc(pool, new_capacity);                                             \
  if(nil == p) {                                                                           \
    return_stmt;                                                                           \
  }                                                                                        \
                                                                                           \
  CALL_AFTER(conn, completed, &shttp_response_pool_free, p);                               \
                                                                                           \
  conn_outgoing(conn).head_buffer.str = p;                                                 \
  conn_outgoing(conn).head_buffer.capacity = new_capacity;                                 \
  sbuffer_length_set(conn_outgoing(conn).head_buffer, 0);                                  \
 
#define head_buffer_append_delimiter(buf)                   \
  memcpy(buf, SHTTP_DELIMITER_STR, SHTTP_DELIMITER_LEN)

#define head_buffer_append_crlf(buf)                        \
  memcpy(buf, SHTTP_CRLF_STR, SHTTP_CRLF_LEN)



static inline shttp_res _shttp_response_write_header_copy(shttp_connection_internal_t *conn,
    const char *buf,
    size_t     buf_len) {
  spool_t                              *pool;
  size_t                               new_capacity;
  char                                 *p;

  pool = conn_response_pool(conn);
  try_realloc_head_buffer(conn, buf_len, return SHTTP_RES_MEMORY);
mem_copy_begin:
  memcpy(p, buf, buf_len);
  sbuffer_length_add(conn_outgoing(conn).head_buffer, buf_len);
  return SHTTP_RES_OK;
}


static inline shttp_res _shttp_response_write_header_format(shttp_connection_internal_t *conn,
    size_t     estimate_length,
    const char *fmt,
    ...) {
  spool_t                              *pool;
  size_t                               new_capacity;
  char                                 *p;
  va_list                              args;
  int                                  res;
  boolean                              tried;

  va_start(args, fmt);
  pool = conn_response_pool(conn);
  tried = false;

#define return_st1 va_end(args); return SHTTP_RES_MEMORY

retry:
  try_realloc_head_buffer(conn, estimate_length, return_st1);
mem_copy_begin:
  res = vsnprintf(p, new_capacity, fmt, args);
  if(-1 == res) {
    if(!tried) {
      tried = true;
      estimate_length = vscprintf(fmt, args);
      goto retry;
    }
    va_end(args);
    return SHTTP_RES_PRINTF;
  }
  sbuffer_length_add(conn_outgoing(conn).head_buffer, res);
  va_end(args);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_write_header(shttp_connection_t *external,
    const char *key,
    size_t     key_len,
    const char *value,
    size_t     value_len) {
  shttp_connection_internal_t          *conn;
  size_t                               estimate_length, new_capacity;
  spool_t                              *pool;
  char                                 *p;

  conn = (shttp_connection_internal_t*)external->internal;

  if(SHTTP_RES_OK != conn_outgoing(conn).failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == conn_response(conn).head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  if(1 == conn_outgoing(conn).is_body_end) {
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }

  do_predefine_headers(conn, key, key_len, value, value_len);

  if(conn_request(conn).headers.length >= conn_outgoing(conn).headers.capacity) {
    return SHTTP_RES_HEAD_TOO_LARGE;
  }

  pool = conn_response_pool(conn);
  estimate_length = key_len + SHTTP_DELIMITER_LEN + value_len + SHTTP_CRLF_LEN;
  try_realloc_head_buffer(conn, estimate_length, return SHTTP_RES_MEMORY);

mem_copy_begin:
  conn_request(conn).headers.array[conn_request(conn).headers.length].key.str = p;
  conn_request(conn).headers.array[conn_request(conn).headers.length].key.len = key_len;
  memcpy(p, key, key_len);
  p += key_len;
  head_buffer_append_delimiter(p);
  p += SHTTP_DELIMITER_LEN;

  conn_request(conn).headers.array[conn_request(conn).headers.length].val.str = p;
  conn_request(conn).headers.array[conn_request(conn).headers.length].val.len = value_len;
  memcpy(p, value, value_len);
  p += value_len;
  head_buffer_append_crlf(p);
  p += SHTTP_CRLF_LEN;

  sbuffer_length_add(conn_outgoing(conn).head_buffer, p- (sbuffer_unused_addr(conn_outgoing(conn).head_buffer)));
  conn_request(conn).headers.length += 1;
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_write_header_format(shttp_connection_t *external,
    const char *key,
    size_t     key_len,
    const char *fmt,
    ...) {
  shttp_connection_internal_t          *conn;
  size_t                               estimate_length, new_capacity;
  int                                  res;
  spool_t                              *pool;
  char                                 *p;
  va_list                              args;
  boolean                              tried;
  shttp_res                            rc;

  conn = (shttp_connection_internal_t*)external->internal;

  if(SHTTP_RES_OK != conn_outgoing(conn).failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == conn_response(conn).head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  if(1 == conn_outgoing(conn).is_body_end) {
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }

  if(conn_request(conn).headers.length >= conn_outgoing(conn).headers.capacity) {
    return SHTTP_RES_HEAD_TOO_LARGE;
  }

  va_start(args, fmt);

  tried = false;
  pool = conn_response_pool(conn);
  estimate_length = key_len + SHTTP_DELIMITER_LEN + strlen(fmt) + 32 + SHTTP_CRLF_LEN;

retry:
  try_realloc_head_buffer(conn, estimate_length, goto failed);

mem_copy_begin:
  new_capacity = sbuffer_unused_size(conn_outgoing(conn).head_buffer);
  p = sbuffer_unused_addr(conn_outgoing(conn).head_buffer);
  memcpy(p, key, key_len);
  head_buffer_append_delimiter(p + key_len);
  res = vsnprintf(p + key_len + SHTTP_DELIMITER_LEN,
                  new_capacity -key_len - SHTTP_DELIMITER_LEN, fmt, args);
  if(-1 == res) {
    if(tried) {
      rc = SHTTP_RES_PRINTF;
      goto failed;
    }
    tried = true;
    estimate_length = key_len + SHTTP_DELIMITER_LEN + vscprintf(fmt, args) + SHTTP_CRLF_LEN;
    goto retry;
  }

  do_predefine_headers(conn, key, key_len, p + key_len + SHTTP_DELIMITER_LEN, res);

  estimate_length = key_len + SHTTP_DELIMITER_LEN + res + SHTTP_CRLF_LEN;
  if(new_capacity >= estimate_length) {
    if(tried) {
      rc = SHTTP_RES_PRINTF;
      goto failed;
    }
    tried = true;
    estimate_length = key_len + SHTTP_DELIMITER_LEN + vscprintf(fmt, args) + SHTTP_CRLF_LEN;
    goto retry;
  }
  head_buffer_append_crlf(p + key_len + SHTTP_DELIMITER_LEN + res);
  sbuffer_length_add(conn_outgoing(conn).head_buffer, estimate_length);

  conn_request(conn).headers.array[conn_request(conn).headers.length].key.str = p;
  conn_request(conn).headers.array[conn_request(conn).headers.length].key.len = key_len;
  conn_request(conn).headers.array[conn_request(conn).headers.length].val.str = p + key_len + SHTTP_DELIMITER_LEN;
  conn_request(conn).headers.array[conn_request(conn).headers.length].val.len = res;

  conn_request(conn).headers.length += 1;

  va_end(args);
  return SHTTP_RES_OK;
failed:
  va_end(args);
  return rc;
}

#define SHTTP_CONNECTION_CLOSE_STR           "close"
#define SHTTP_CONNECTION_CLOSE_LEN           5
#define SHTTP_CONNECTION_KEEP_ALIVE_STR      "keep-alive"
#define SHTTP_CONNECTION_KEEP_ALIVE_LEN      10
#define SHTTP_CONNECTION_MAX_LEN             10

static shttp_res _shttp_response_send_predefine_headers(shttp_connection_internal_t *conn) {
  cstring_t     *status_code;
  int           res;
  shttp_res     rc;
  sbuf_t        buf;

  status_code = shttp_status_code_text(conn_response(conn).status_code);
  if(nil == status_code) {
    return SHTTP_RES_ERROR_STATUS_CODE;
  }

#define _shttp_header_copy(conn, buf, buf_len)                 \
  rc = _shttp_response_write_header_copy(conn, buf, buf_len);  \
  if(SHTTP_RES_OK != rc) {                                     \
    return rc;                                                 \
  }

  //  Connection header
  _shttp_header_copy(conn,
                     "Connection" SHTTP_DELIMITER_STR,
                     10 + SHTTP_DELIMITER_LEN);

  switch(conn_response(conn).close_connection) {
  case SHTTP_CLOSE_CONNECTION_NONE:
    if(0 == http_should_keep_alive(&conn->parser)) {
      _shttp_header_copy(conn,
                         SHTTP_CONNECTION_CLOSE_STR SHTTP_CRLF_STR,
                         SHTTP_CONNECTION_CLOSE_LEN + SHTTP_CRLF_LEN);
    } else {
      _shttp_header_copy(conn,
                         SHTTP_CONNECTION_KEEP_ALIVE_STR SHTTP_CRLF_STR,
                         SHTTP_CONNECTION_KEEP_ALIVE_LEN + SHTTP_CRLF_LEN);
    }
    break;
  case SHTTP_CLOSE_CONNECTION_TRUE:
    _shttp_header_copy(conn,
                       SHTTP_CONNECTION_KEEP_ALIVE_STR SHTTP_CRLF_STR,
                       SHTTP_CONNECTION_KEEP_ALIVE_LEN + SHTTP_CRLF_LEN);
    break;
  default:
    _shttp_header_copy(conn,
                       SHTTP_CONNECTION_CLOSE_STR SHTTP_CRLF_STR,
                       SHTTP_CONNECTION_CLOSE_LEN + SHTTP_CRLF_LEN);
    break;
  }

  // Content-Type Header
  _shttp_header_copy(conn,
                     "Content-Type" SHTTP_DELIMITER_STR,
                     12 + SHTTP_DELIMITER_LEN);
  _shttp_header_copy(conn,
                     conn_response(conn).content_type.str,
                     conn_response(conn).content_type.len);
  _shttp_header_copy(conn, SHTTP_CRLF_STR, SHTTP_CRLF_LEN);

  // Transfer-Encoding AND Content-Length Header
  if(SHTTP_THUNKED_TRUE == conn_response(conn).chunked) {
    _shttp_header_copy(conn,
                       "Transfer-Encoding" SHTTP_DELIMITER_STR "chunked" SHTTP_CRLF_STR,
                       17 + SHTTP_DELIMITER_LEN + 7 + SHTTP_CRLF_LEN);
  } else if(((size_t)-1) != conn_response(conn).content_length) { // Content-Length Header
    rc = _shttp_response_write_header_format(conn, 28,
         "Content-Length" SHTTP_DELIMITER_STR "%d" SHTTP_CRLF_STR,
         conn_response(conn).content_length);
    if(SHTTP_RES_OK != rc) {
      return rc;
    }
  }
  if(0 == conn_outgoing(conn).head_write_buffers.length) {
    conn_outgoing(conn).head_write_buffers.length = 1;
  }

  _shttp_header_copy(conn, SHTTP_CRLF_STR, SHTTP_CRLF_LEN);
  WRITE_TO_UV_BUF(conn, head_write_buffers,
                  conn_outgoing(conn).head_buffer.str,
                  sbuffer_length(conn_outgoing(conn).head_buffer));
  sbuffer_zero(conn_outgoing(conn).head_buffer);

  res = spool_prepare_alloc(conn_response_pool(conn), &buf);
  if(SHTTP_RES_OK != res) {
    return res;
  }

  res = snprintf(buf.str, buf.len, "HTTP/%d.%d %d ",
                 conn_response(conn).http_major,
                 conn_response(conn).http_minor,
                 conn_response(conn).status_code);
  if(-1 == res) {
    goto failed;
  }

  if((res + status_code->len + SHTTP_CRLF_LEN) > buf.len) {
    goto failed;
  }
  memcpy(buf.str + res, status_code->str, status_code->len);
  head_buffer_append_crlf(buf.str + res + status_code->len);

  conn_outgoing(conn).head_write_buffers.array[0].base = buf.str;
  conn_outgoing(conn).head_write_buffers.array[0].len = (ULONG)(res + status_code->len + SHTTP_CRLF_LEN);
  if(0 == conn_outgoing(conn).head_write_buffers.length) {
    conn_outgoing(conn).head_write_buffers.length = 1;
  }

  spool_commit_alloc(conn_response_pool(conn), res + status_code->len + SHTTP_CRLF_LEN);
  return SHTTP_RES_OK;
failed:
  spool_rollback_alloc(conn_response_pool(conn));
  return SHTTP_RES_MEMORY;
}

DLL_VARIABLE shttp_res shttp_response_write(shttp_connection_t *external,
    const char *data,
    size_t length,
    shttp_write_cb cb,
    void *cb_data) {
  shttp_connection_internal_t          *conn;
  conn = (shttp_connection_internal_t*)external->internal;

  conn_response(conn).content_length += length;

  if(SHTTP_RES_OK != conn_outgoing(conn).failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == conn_outgoing(conn).is_body_end) {
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }

  WRITE_TO_UV_BUF(conn, body_write_buffers, (char*)data, length);
  if(nil != cb) {
    CALL_AFTER(conn, data_writed, cb, cb_data);
  }
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_end(shttp_connection_t *external) {
  shttp_res                            rc;
  shttp_connection_internal_t          *conn;
  conn = (shttp_connection_internal_t*)external->internal;

  if(SHTTP_RES_OK != conn_outgoing(conn).failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 != conn_response(conn).head_writed) {
    assert(0 == conn_outgoing(conn).is_body_end);
    rc = _shttp_response_send_predefine_headers(conn);
    if(SHTTP_RES_OK != rc) {
      ERR("callback: write predefine header failed.");
      conn_outgoing(conn).failed_code = rc;
      return rc;
    }
    conn_response(conn).head_writed = 1;
  }


  if(nil != conn_outgoing(conn).content_type.str) {
    conn_response_free(conn, conn_outgoing(conn).content_type.str);
    conn_outgoing(conn).content_type.len = 0;
    conn_outgoing(conn).content_type.str = nil;
  }


  assert(0 == conn_outgoing(conn).is_body_end);
  conn_outgoing(conn).is_body_end = 1;
  return SHTTP_RES_OK;
}

void _shttp_response_send_ready_data(shttp_connection_internal_t *conn,
                                     uv_close_cb on_disconnect,
                                     uv_write_cb on_head_writed,
                                     uv_write_cb on_data_writed) {
  int rc;

  if(SHTTP_RES_OK != conn_outgoing(conn).failed_code) {
    _shttp_response_send_error_message_format(conn, on_disconnect,
        INTERNAL_ERROR_FROMAT, shttp_strerr(conn_outgoing(conn).failed_code));
    return;
  }

  if(0 == conn_outgoing(conn).is_body_end &&
      0 == conn_response(conn).chunked) {
    ERR("callback: chunked must is true while body is not completed.");
    conn_outgoing(conn).failed_code = SHTTP_RES_BODY_NOT_COMPLETE;
    _shttp_response_send_error_message(conn, on_disconnect,
                                       BODY_NOT_COMPLETE, strlen(BODY_NOT_COMPLETE));
    return;
  }

  if(1 != conn_response(conn).head_writed) {
    assert(0 == conn_outgoing(conn).is_body_end);
    rc = _shttp_response_send_predefine_headers(conn);
    if(SHTTP_RES_OK != rc) {
      ERR("callback: write predefine header failed.");
      goto failed;
    }
    conn_response(conn).head_writed = 1;
  }

  if(0 == conn_outgoing(conn).head_write_buffers.length) {
    if(0 == conn_outgoing(conn).body_write_buffers.length) {
      ERR("callback: write buffers is empty.");
      rc = SHTTP_RES_RESPONSE_BODY_IS_EMPTY;
      goto failed;
    }

    rc = uv_write(&conn_outgoing(conn).write_req,
                  (uv_stream_t*)&conn->uv_handle,
                  &conn_outgoing(conn).body_write_buffers.array[0],
                  (unsigned long)conn_outgoing(conn).body_write_buffers.length,
                  on_data_writed);
  } else {
    rc = uv_write(&conn_outgoing(conn).write_req,
                  (uv_stream_t*)&conn->uv_handle,
                  &conn_outgoing(conn).head_write_buffers.array[0],
                  (unsigned long)conn_outgoing(conn).head_write_buffers.length,
                  on_head_writed);
  }
  if(0 == rc) {
    return;
  }
  ERR("write: %s", uv_strerror(rc));
failed:
  conn_outgoing(conn).failed_code = rc;
  _shttp_response_call_hooks_for_failed(conn);
  uv_close((uv_handle_t*) &conn->uv_handle, on_disconnect);
  return;
}

#ifdef __cplusplus
}
#endif