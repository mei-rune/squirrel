#include "squirrel_config.h"
#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif


#define to_internal(external)          ((shttp_context_internal_t *) external->internal)
#define ctx_data_buffer(conn)          ctx_outgoing(ctx).body_write_buffers
#define ctx_last_data_buffer(conn)     ctx_data_buffer(conn).array[ctx_data_buffer(conn).length - 1]
#define ctx_response_last_header(conn) (response)->headers.array[(response)->headers.length]

#define HEAD_AND_BODY_IS_NOT_END(ctx)                     \
  if(SHTTP_RES_OK != ctx->outgoing.failed_code) {         \
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;             \
  }                                                       \
  if(1 == ctx->outgoing.head_writed) {                    \
    return SHTTP_RES_HEAD_WRITED;                         \
  }                                                       \
  if(1 == ctx->outgoing.is_body_end) {                    \
    shttp_assert(0 == ctx->outgoing.is_body_end);         \
    return SHTTP_RES_RESPONSE_ALREADY_END;                \
  }

#define return_stmt(x)  return (x);
#define goto_stmt(x)  rc = (x); goto failed

#define write_to_uv_buffers(conn, write_buffers, buf, buf_len, return_st)                                          \
    if(ctx_outgoing(ctx).write_buffers.capacity <= ctx_outgoing(ctx).write_buffers.length) {                       \
      return_st(SHTTP_RES_INSUFFICIENT_WRITE_BUFFER);                                                              \
    }                                                                                                              \
    ctx_outgoing(ctx).write_buffers.array[ctx_outgoing(ctx).write_buffers.length].base = (buf);                    \
    ctx_outgoing(ctx).write_buffers.array[ctx_outgoing(ctx).write_buffers.length].len = (ULONG)(buf_len);          \
    ctx_outgoing(ctx).write_buffers.length ++

#define WRITE_TO_UV_BUF(ctx, write_buffers, buf, buf_len)  write_to_uv_buffers(ctx, write_buffers, buf, buf_len, return_stmt)


#define call_after(conn, action, func, func_data, return_st)                                                       \
    if(ctx_outgoing(ctx).call_after_##action.capacity <= ctx_outgoing(ctx).call_after_##action.length) {           \
      return_st(SHTTP_RES_INSUFFICIENT_WRITE_CB_BUFFER);                                                           \
    }                                                                                                              \
    ctx_outgoing(ctx).call_after_##action.array[ctx_outgoing(ctx).call_after_##action.length].cb = (func);         \
    ctx_outgoing(ctx).call_after_##action.array[ctx_outgoing(ctx).call_after_##action.length].data = (func_data);  \
    ctx_outgoing(ctx).call_after_##action.length ++

#define CALL_AFTER(ctx, action, func, func_data) call_after(ctx, action, func, func_data, return_stmt)

#define ctx_response_malloc(ctx, size)          spool_malloc(&(ctx)->pool, (size))
#define ctx_response_realloc(ctx, p, size)      spool_realloc(&(ctx)->pool, (p), (size))
#define ctx_response_try_realloc(ctx, p, size)  spool_try_realloc(&(ctx)->pool, (p), (size))
#define ctx_response_free(ctx, p)               spool_free(&(ctx)->pool, (p))
#define ctx_response_pool(ctx)                  &(ctx)->pool

#define copy_content_type(ctx, response, txt, txt_len)                                     \
  if(txt_len > ctx->outgoing.content_type.len) {                                           \
    if(0 == ctx->outgoing.content_type.len) {                                              \
      ctx->outgoing.content_type.len = sl_max(128, txt_len);                               \
      ctx->outgoing.content_type.str = (char*)ctx_response_malloc(ctx,                     \
                                                     sl_max(128, txt_len));                \
      if(nil == ctx->outgoing.content_type.str) {                                          \
        return SHTTP_RES_MEMORY;                                                           \
      }                                                                                    \
    } else {                                                                               \
      ctx->outgoing.content_type.len = txt_len;                                            \
      ctx->outgoing.content_type.str = (char*)ctx_response_realloc(ctx,                    \
        ctx->outgoing.content_type.str, txt_len);                                          \
      if(nil == ctx->outgoing.content_type.str) {                                          \
        return SHTTP_RES_MEMORY;                                                           \
      }                                                                                    \
    }                                                                                      \
  }                                                                                        \
  response->content_type.str = ctx->outgoing.content_type.str;                             \
  response->content_type.len = txt_len;                                                    \
  memcpy(ctx->outgoing.content_type.str, txt, txt_len)



#ifdef SHTTP_THREAD_SAFE
#define THREAD_IS_ZERO(conn)   0 != ctx_outgoing(ctx).thread_id &&
#else
#define THREAD_IS_ZERO(conn)
#endif

#define IS_THREAD_SAFE(conn)                                                               \
  if(0 == shttp_atomic_read32(&ctx_outgoing(ctx).is_async)) {                              \
    shttp_assert_for_cb(0 == shttp_atomic_read32(&ctx_outgoing(ctx).is_async));            \
    return SHTTP_RES_NOT_ASYNC;                                                            \
  }                                                                                        \
  if(THREAD_IS_ZERO(conn) shttp_atomic_read32(&ctx_outgoing(ctx).thread_id) != uv_thread_self()) {  \
    shttp_assert_for_cb(ctx_outgoing(ctx).thread_id == uv_thread_self());                  \
    return SHTTP_RES_THREAD_SAFE;                                                          \
  }                                                                                        \
  if(0 != shttp_atomic_read32(&ctx_outgoing(ctx).is_writing)) {                            \
    shttp_assert_for_cb(0 == shttp_atomic_read32(&ctx_outgoing(ctx).is_writing));          \
    return SHTTP_RES_THREAD_SAFE;                                                          \
  }

#define do_predefine_headers(ctx, response, key, key_len, value, value_len)                \
  if(12 == key_len && 0 == strncmp("Content-Type", key, key_len)) {                        \
    copy_content_type(ctx, (response), value, value_len);                                  \
    return SHTTP_RES_OK;                                                                   \
  } else if(17 == key_len && 0 == strncmp("Transfer-Encoding", key, key_len)) {            \
    if(7 == value_len && 0 == strncasecmp("chunked", value, value_len)) {                  \
      if(SHTTP_THUNKED_NONE ==  (response)->chunked ||                                     \
        SHTTP_THUNKED_TRUE ==   (response)->chunked) {                                     \
        (response)->chunked = SHTTP_THUNKED_TRUE;                                          \
        return SHTTP_RES_OK;                                                               \
      }                                                                                    \
    }                                                                                      \
    return SHTTP_RES_HEAD_TRANSFER_ENCODING;                                               \
  } else if(10 == key_len && 0 == strncmp("Connection", key, key_len)) {                   \
    if(5 == value_len && 0 == strncasecmp("close", value, value_len)) {                    \
       if(SHTTP_CLOSE_CONNECTION_NONE == (response)->close_connection ||                   \
        SHTTP_CLOSE_CONNECTION_TRUE ==  (response)->close_connection) {                    \
        (response)->close_connection = SHTTP_CLOSE_CONNECTION_TRUE;                        \
        return SHTTP_RES_OK;                                                               \
      }                                                                                    \
    } else if(10 == value_len && 0 == strncasecmp("keep-alive", value, value_len)) {       \
      if(SHTTP_CLOSE_CONNECTION_NONE == (response)->close_connection ||                    \
        SHTTP_CLOSE_CONNECTION_FALSE ==  (response)->close_connection) {                   \
        (response)->close_connection = SHTTP_CLOSE_CONNECTION_FALSE;                       \
        return SHTTP_RES_OK;                                                               \
      }                                                                                    \
    }                                                                                      \
    return SHTTP_RES_HEAD_CONNECTION;                                                      \
  }


#define try_realloc_head_buffer(ctx, estimate_length, return_st)                           \
  if(nil != ctx_outgoing(ctx).head_buffer.str) {                                           \
    new_capacity = sbuffer_unused_size(ctx_outgoing(ctx).head_buffer);                     \
    if(estimate_length < new_capacity) {                                                   \
      p = sbuffer_unused_addr(ctx_outgoing(ctx).head_buffer);                              \
      goto mem_copy_begin;                                                                 \
    }                                                                                      \
                                                                                           \
    new_capacity = ctx_outgoing(ctx).head_buffer.capacity +                                \
      shttp_align(sl_max(estimate_length, 256), shttp_cacheline_size);                     \
                                                                                           \
    if(nil != spool_try_realloc(ctx->external.pool,                                        \
      ctx_outgoing(ctx).head_buffer.str,                                                   \
      new_capacity)) {                                                                     \
      ctx_outgoing(ctx).head_buffer.capacity = new_capacity;                               \
      p = sbuffer_unused_addr(ctx_outgoing(ctx).head_buffer);                              \
      goto mem_copy_begin;                                                                 \
    }                                                                                      \
                                                                                           \
    if(0 == ctx_outgoing(ctx).head_write_buffers.length) {                                 \
       /* this is for first line of response.*/                                            \
      ctx_outgoing(ctx).head_write_buffers.length = 1;                                     \
    }                                                                                      \
    if(0 != sbuffer_length(ctx_outgoing(ctx).head_buffer)) {                               \
      write_to_uv_buffers(ctx, head_write_buffers,                                         \
        ctx_outgoing(ctx).head_buffer.str,                                                 \
        sbuffer_length(ctx_outgoing(ctx).head_buffer), return_st);                         \
    }                                                                                      \
  }                                                                                        \
                                                                                           \
  new_capacity = spool_excepted(sl_max(estimate_length, 256));                             \
  p = (char*)spool_malloc(ctx->external.pool, new_capacity);                               \
  if(nil == p) {                                                                           \
    return_st(SHTTP_RES_MEMORY);                                                           \
  }                                                                                        \
                                                                                           \
  call_after(ctx, completed, &shttp_response_pool_free, p, return_st);                     \
                                                                                           \
  ctx_outgoing(ctx).head_buffer.str = p;                                                   \
  ctx_outgoing(ctx).head_buffer.capacity = new_capacity;                                   \
  sbuffer_length_set(ctx_outgoing(ctx).head_buffer, 0);                                    \
 
#define head_buffer_append_delimiter(buf)                                                  \
  memcpy(buf, SHTTP_DELIMITER_STR, SHTTP_DELIMITER_LEN)

#define head_buffer_append_crlf(buf)                                                       \
  memcpy(buf, SHTTP_CRLF_STR, SHTTP_CRLF_LEN)


static inline shttp_res _shttp_response_write_header_copy(shttp_context_internal_t *ctx,
    const char *buf,
    size_t     buf_len) {
  size_t                               new_capacity;
  char                                 *p;

  try_realloc_head_buffer(ctx, buf_len, return_stmt);
mem_copy_begin:
  memcpy(p, buf, buf_len);
  sbuffer_length_add(ctx_outgoing(ctx).head_buffer, buf_len);
  return SHTTP_RES_OK;
}

static inline shttp_res _shttp_response_write_header_format(shttp_context_internal_t *ctx,
    size_t estimate_length, const char *fmt, ...) {
  size_t                               new_capacity;
  char                                 *p;
  va_list                              args;
  int                                  res;
  boolean                              tried;

  va_start(args, fmt);
  tried = false;

#define return_st1(x) va_end(args); return (x)

retry:
  try_realloc_head_buffer(ctx, estimate_length, return_st1);
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
  sbuffer_length_add(ctx_outgoing(ctx).head_buffer, res);
  va_end(args);
  return SHTTP_RES_OK;
}

//static inline shttp_res _shttp_response_write_header_helper(shttp_outgoing_t *conn,
//    const char *key, size_t key_len, const char *value, size_t value_len) {
//  size_t                               estimate_length, new_capacity;
//  spool_t                              *pool;
//  char                                 *p;
//
//  if(SHTTP_RES_OK != ctx_outgoing(ctx).failed_code) {
//    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
//  }
//  if(1 == conn_response(conn).head_writed) {
//    return SHTTP_RES_HEAD_WRITED;
//  }
//  if(1 == ctx_outgoing(ctx).is_body_end) {
//    shttp_assert(0 == ctx_outgoing(ctx).is_body_end);
//    return SHTTP_RES_RESPONSE_ALREADY_END;
//  }
//
//  do_predefine_headers(conn, key, key_len, value, value_len);
//
//  if(conn_response(conn).headers.length >= ctx_outgoing(ctx).headers.capacity) {
//    return SHTTP_RES_HEAD_TOO_LARGE;
//  }
//
//  pool = ctx_response_pool(conn);
//  estimate_length = key_len + SHTTP_DELIMITER_LEN + value_len + SHTTP_CRLF_LEN;
//  try_realloc_head_buffer(conn, estimate_length, return SHTTP_RES_MEMORY);
//
//mem_copy_begin:
//  conn_response_last_header(conn).key.str = p;
//  conn_response_last_header(conn).key.len = key_len;
//  memcpy(p, key, key_len);
//  p += key_len;
//  head_buffer_append_delimiter(p);
//  p += SHTTP_DELIMITER_LEN;
//
//  conn_response_last_header(conn).val.str = p;
//  conn_response_last_header(conn).val.len = value_len;
//  memcpy(p, value, value_len);
//  p += value_len;
//  head_buffer_append_crlf(p);
//  p += SHTTP_CRLF_LEN;
//
//  sbuffer_length_add(ctx_outgoing(ctx).head_buffer,
//                     p - (sbuffer_unused_addr(ctx_outgoing(ctx).head_buffer)));
//  conn_response(conn).headers.length += 1;
//  return SHTTP_RES_OK;
//}
//
//static inline shttp_res _shttp_response_format_header_helper(shttp_outgoing_t *conn,
//    const char *key,
//    size_t      key_len,
//    const char *fmt,
//    va_list     args) {
//  size_t                               estimate_length, new_capacity;
//  int                                  res;
//  spool_t                              *pool;
//  char                                 *p;
//  boolean                              tried;
//
//  if(SHTTP_RES_OK != ctx_outgoing(ctx).failed_code) {
//    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
//  }
//  if(1 == conn_response(conn).head_writed) {
//    return SHTTP_RES_HEAD_WRITED;
//  }
//  if(1 == ctx_outgoing(ctx).is_body_end) {
//    shttp_assert(0 == ctx_outgoing(ctx).is_body_end);
//    return SHTTP_RES_RESPONSE_ALREADY_END;
//  }
//
//  if(conn_response(conn).headers.length >= ctx_outgoing(ctx).headers.capacity) {
//    return SHTTP_RES_HEAD_TOO_LARGE;
//  }
//
//  tried = false;
//  pool = ctx_response_pool(conn);
//  estimate_length = key_len + SHTTP_DELIMITER_LEN + strlen(fmt) + 32 + SHTTP_CRLF_LEN;
//
//retry:
//  try_realloc_head_buffer(conn, estimate_length, return SHTTP_RES_MEMORY);
//
//mem_copy_begin:
//  new_capacity = sbuffer_unused_size(ctx_outgoing(ctx).head_buffer);
//  p = sbuffer_unused_addr(ctx_outgoing(ctx).head_buffer);
//  res = vsnprintf(p + key_len + SHTTP_DELIMITER_LEN,
//                  new_capacity - key_len - SHTTP_DELIMITER_LEN - SHTTP_CRLF_LEN, fmt, args);
//  if(-1 == res) {
//    if(tried) {
//      return SHTTP_RES_PRINTF;
//    }
//    tried = true;
//    estimate_length = key_len + SHTTP_DELIMITER_LEN + vscprintf(fmt, args) + SHTTP_CRLF_LEN;
//    goto retry;
//  }
//  do_predefine_headers(conn, key, key_len,
//                       p + key_len + SHTTP_DELIMITER_LEN, res);
//
//  conn_response_last_header(conn).key.str = p;
//  conn_response_last_header(conn).key.len = key_len;
//  memcpy(p, key, key_len);
//  p += key_len;
//  head_buffer_append_delimiter(p);
//  p += SHTTP_DELIMITER_LEN;
//  p += res;
//  conn_response_last_header(conn).val.str = p;
//  conn_response_last_header(conn).val.len = res;
//  head_buffer_append_crlf(p);
//  p += SHTTP_CRLF_LEN;
//
//  sbuffer_length_add(ctx_outgoing(ctx).head_buffer,
//                     p - sbuffer_unused_addr(ctx_outgoing(ctx).head_buffer) );
//  conn_response(conn).headers.length += 1;
//
//  return SHTTP_RES_OK;
//}

#define SHTTP_CONNECTION_CLOSE_STR           "close"
#define SHTTP_CONNECTION_CLOSE_LEN           5
#define SHTTP_CONNECTION_KEEP_ALIVE_STR      "keep-alive"
#define SHTTP_CONNECTION_KEEP_ALIVE_LEN      10
#define SHTTP_CONNECTION_MAX_LEN             10


#ifdef __cplusplus
};
#endif