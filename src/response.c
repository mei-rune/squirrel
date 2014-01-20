#include "squirrel_config.h"

#include <assert.h>

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * HTTP Utility Functions
 ****************************************************************************/

/** @name HTTP Status
 */
/**@{*/

#define XX(num, string) {0, string}

/* Informational 1xx */
static size_t HTTP_STATUS_100_LEN;
static cstring_t HTTP_STATUS_100[] = {
  XX(100, "Continue"),
  XX(101, "Switching Protocols"),
  XX(102, "Processing"),
  XX(103, nil)
};

/* Successful 2xx */
static size_t HTTP_STATUS_200_LEN;
static cstring_t HTTP_STATUS_200[] = {
  XX(200, "OK"),
  XX(201, "Created"),
  XX(202, "Accepted"),
  XX(203, "Non-Authoritative Information"),
  XX(204, "No Content"),
  XX(205, "Reset Content"),
  XX(206, "Partial Content"),
  XX(207, "Multi-Status"),
  XX(208, nil)
};

/* Redirection 3xx */
static size_t HTTP_STATUS_300_LEN;
static cstring_t HTTP_STATUS_300[] = {
  XX(300, "Multiple Choices"),
  XX(301, "Moved Permanently"),
  XX(302, "Moved Temporarily"),
  XX(303, "See Other"),
  XX(304, "Not Modified"),
  XX(305, "Use Proxy"),
  XX(307, "Temporary Redirect"),
  XX(308, nil)
};

/* Client Error 4xx */
static size_t HTTP_STATUS_400_LEN;
static cstring_t HTTP_STATUS_400[] = {
  XX(400, "Bad Request"),
  XX(401, "Unauthorized"),
  XX(402, "Payment Required"),
  XX(403, "Forbidden"),
  XX(404, "Not Found"),
  XX(405, "Method Not Allowed"),
  XX(406, "Not Acceptable"),
  XX(407, "Proxy Authentication Required"),
  XX(408, "Request Time-out"),
  XX(409, "Conflict"),
  XX(410, "Gone"),
  XX(411, "Length Required"),
  XX(412, "Precondition Failed"),
  XX(413, "Request Entity Too Large"),
  XX(414, "Request-URI Too Large"),
  XX(415, "Unsupported Media Type"),
  XX(416, "Requested Range Not Satisfiable"),
  XX(417, "Expectation Failed"),
  XX(418, "I'm a teapot"),
  XX(422, "Unprocessable Entity"),
  XX(423, "Locked"),
  XX(424, "Failed Dependency"),
  XX(425, "Unordered Collection"),
  XX(426, "Upgrade Required"),
  XX(428, "Precondition Required"),
  XX(429, "Too Many Requests"),
  XX(431, "Request Header Fields Too Large"),
  XX(432, nil)
};

/* Server Error 5xx */
static size_t HTTP_STATUS_500_LEN;
static cstring_t HTTP_STATUS_500[] = {
  XX(500, "Internal Server Error"),
  XX(501, "Not Implemented"),
  XX(502, "Bad Gateway"),
  XX(503, "Service Unavailable"),
  XX(504, "Gateway Time-out"),
  XX(505, "HTTP Version Not Supported"),
  XX(506, "Variant Also Negotiates"),
  XX(507, "Insufficient Storage"),
  XX(509, "Bandwidth Limit Exceeded"),
  XX(510, "Not Extended"),
  XX(511, "Network Authentication Required"),
  XX(512, nil)
};

static cstring_t UNKNOWN_STATUS_CODE = {0, "UNKNOWN STATUS CODE"};

#undef XX
/**@}*/



void _shttp_status_init() {
  size_t i;

#define XX(num)   for(i =0; nil != HTTP_STATUS_##num##[i].str; i ++) {    \
    HTTP_STATUS_##num##[i].len = strlen(HTTP_STATUS_##num##[i].str);      \
  }                                                                       \
  HTTP_STATUS_##num##_LEN = i

  XX(100);
  XX(200);
  XX(300);
  XX(400);
  XX(500);

#undef XX
}

DLL_VARIABLE cstring_t* shttp_status_code_text(int status) {
#define XX(num)  if(num >= status) {               \
  if(HTTP_STATUS_##num##_LEN > (status - num)) {   \
       return &HTTP_STATUS_##num[status - num];    \
    }                                              \
  }

  XX(100);
  XX(200);
  XX(300);
  XX(400);
  XX(500);

#undef XX
  return &UNKNOWN_STATUS_CODE;
}




#define WRITE_TO_UV_BUF(inner_conn, write_buffers, buf, buf_len)                                               \
    if(inner_conn->outgoing.write_buffers.capacity <= inner_conn->outgoing.write_buffers.length) {             \
      return SHTTP_RES_INSUFFICIENT_WRITE_BUFFER;                                                              \
    }                                                                                                          \
    inner_conn->outgoing.write_buffers.array[inner_conn->outgoing.write_buffers.length].base = (buf);          \
    inner_conn->outgoing.write_buffers.array[inner_conn->outgoing.write_buffers.length].len = (buf_len);       \
    inner_conn->outgoing.write_buffers.length ++


#define CALL_AFTER_WRITED(inner, func, func_data) if(inner->outgoing.call_after_writed.capacity <=             \
                        inner->outgoing.call_after_writed.length) {                                            \
      return SHTTP_RES_INSUFFICIENT_WRITE_CB_BUFFER;                                                           \
    }                                                                                                          \
    inner->outgoing.call_after_writed.array[inner->outgoing.call_after_writed.length].cb = (func);             \
    inner->outgoing.call_after_writed.array[inner->outgoing.call_after_writed.length].data = (func_data)  ;    \
    inner->outgoing.call_after_writed.length ++

#define conn_malloc(conn, size) spool_malloc((conn)->pool, (size))
#define conn_realloc(inner, p, size) spool_realloc((conn)->pool, (p), (size))
#define conn_free(inner, p) spool_free((conn)->pool, (p))

char* conn_strdup(shttp_connection_t *inner, const char *c, size_t size) {
  char * p;

  p = (char*) conn_malloc(inner, size + 1);
  if(nil == p) {
    return nil;
  }
  memcpy(p, c, size+1);
  return p;
}

#define copy_content_type(txt, txt_len)                                             \
  if(txt_len > inner->outgoing.content_type.len) {                                  \
    if(nil == inner->outgoing.content_type.len) {                                   \
      inner->outgoing.content_type.len = sl_max(128, txt_len);                      \
      inner->outgoing.content_type.str = (char*)spool_malloc(conn->pool,            \
                                                     sl_max(128, txt_len));         \
      if(nil == inner->outgoing.content_type.str) {                                 \
        return SHTTP_RES_MEMORY;                                                    \
      }                                                                             \
    } else {                                                                        \
      inner->outgoing.content_type.len = txt_len;                                   \
      inner->outgoing.content_type.str = (char*)spool_realloc(conn->pool,           \
        inner->outgoing.content_type.str, txt_len);                                 \
      if(nil == inner->outgoing.content_type.str) {                                 \
        return SHTTP_RES_MEMORY;                                                    \
      }                                                                             \
    }                                                                               \
  }                                                                                 \
  conn->response.content_type.str = inner->outgoing.content_type.str;               \
  conn->response.content_type.len = txt_len;                                        \
  memcpy(inner->outgoing.content_type.str, txt, txt_len)


DLL_VARIABLE shttp_res shttp_response_start(shttp_connection_t *conn,
    uint16_t status,
    const char *content_type,
    size_t content_type_len) {
  shttp_connection_internal_t          *inner;

  inner = (shttp_connection_internal_t*)conn->internal;
  if(1 == inner->inner.response.head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  assert(0 == inner->outgoing.is_body_end);

  inner->inner.response.status_code = status;
  copy_content_type(content_type, content_type_len);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_set_chuncked(shttp_connection_t *conn) {
  shttp_connection_internal_t          *inner;
  inner = (shttp_connection_internal_t*)conn->internal;
  if(1 == inner->inner.response.head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  assert(0 == inner->outgoing.is_body_end);
  conn->response.chunked = 1;
  return SHTTP_RES_OK;
}


DLL_VARIABLE void shttp_connection_pool_free (shttp_connection_t *conn, void *data) {
  shttp_connection_internal_t          *inner;
  inner = (shttp_connection_internal_t*)conn->internal;
  spool_free(&inner->pool, data);
}

DLL_VARIABLE void shttp_response_pool_free (shttp_connection_t *conn, void *data) {
  shttp_connection_internal_t          *inner;
  inner = (shttp_connection_internal_t*)conn->internal;
  spool_free(&inner->pool, data);
}

DLL_VARIABLE void shttp_response_c_free (shttp_connection_t *conn, void *data) {
  sl_free(data);
}



#define do_predefine_headers(key, key_len, value, value_len)                               \
  if(12 == key_len && 0 == strncmp("Content-Type", key, key_len)) {                        \
    copy_content_type(value, value_len);                                                   \
    return SHTTP_RES_OK;                                                                   \
  } else if(17 == key_len && 0 == strncmp("Transfer-Encoding", key, key_len)) {            \
    if(7 == value_len && 0 == strncasecmp("chunked", value, value_len)) {                  \
      if(SHTTP_THUNKED_NONE == conn->response.chunked ||                                   \
        SHTTP_THUNKED_TRUE ==  conn->response.chunked) {                                   \
        conn->response.chunked = SHTTP_THUNKED_TRUE;                                       \
        return SHTTP_RES_OK;                                                               \
      }                                                                                    \
    }                                                                                      \
    return SHTTP_RES_HEAD_TRANSFER_ENCODING;                                               \
  } else if(10 == key_len && 0 == strncmp("Connection", key, key_len)) {                   \
    if(5 == value_len && 0 == strncasecmp("close", value, value_len)) {                    \
       if(SHTTP_CLOSE_CONNECTION_NONE == conn->response.close_connection ||                \
        SHTTP_CLOSE_CONNECTION_TRUE ==  conn->response.close_connection) {                 \
        conn->response.close_connection = SHTTP_CLOSE_CONNECTION_TRUE;                     \
        return SHTTP_RES_OK;                                                               \
      }                                                                                    \
    } else if(10 == value_len && 0 == strncasecmp("keep-alive", value, value_len)) {       \
      if(SHTTP_CLOSE_CONNECTION_NONE == conn->response.close_connection ||                 \
        SHTTP_CLOSE_CONNECTION_FALSE ==  conn->response.close_connection) {                \
        conn->response.close_connection = SHTTP_CLOSE_CONNECTION_FALSE;                    \
        return SHTTP_RES_OK;                                                               \
      }                                                                                    \
    }                                                                                      \
    return SHTTP_RES_HEAD_CONNECTION;                                                      \
  }


#define try_realloc_head_buffer(estimate_length, return_stmt)                              \
  if(nil != inner->outgoing.head_buffer.str) {                                             \
    new_capacity = sbuffer_unused_size(inner->outgoing.head_buffer);                       \
    if(estimate_length < new_capacity) {                                                   \
      p = sbuffer_unused_addr(inner->outgoing.head_buffer);                                \
      goto mem_copy_begin;                                                                 \
    }                                                                                      \
                                                                                           \
    new_capacity = inner->outgoing.head_buffer.capacity +                                  \
      shttp_align(sl_max(estimate_length, 256), shttp_cacheline_size);                     \
                                                                                           \
    if(nil != spool_try_realloc(pool,                                                      \
      inner->outgoing.head_buffer.str,                                                     \
      new_capacity)) {                                                                     \
      inner->outgoing.head_buffer.capacity = new_capacity;                                 \
      p = sbuffer_unused_addr(inner->outgoing.head_buffer);                                \
      goto mem_copy_begin;                                                                 \
    }                                                                                      \
                                                                                           \
    if(0 == inner->outgoing.head_write_buffers.length) {                                   \
       /* this is for first line of response.*/                                            \
      inner->outgoing.head_write_buffers.length = 1;                                       \
    }                                                                                      \
    if(0 != sbuffer_length(inner->outgoing.head_buffer)) {                                 \
      WRITE_TO_UV_BUF(inner, head_write_buffers, inner->outgoing.head_buffer.str,          \
        sbuffer_length(inner->outgoing.head_buffer));                                      \
    }                                                                                      \
  }                                                                                        \
                                                                                           \
  new_capacity = spool_excepted(sl_max(estimate_length, 256));                             \
  p = (char*)spool_malloc(pool, new_capacity);                                             \
  if(nil == p) {                                                                           \
    return_stmt;                                                                           \
  }                                                                                        \
                                                                                           \
  CALL_AFTER_WRITED(inner, &shttp_response_pool_free, p);                                  \
                                                                                           \
  inner->outgoing.head_buffer.str = p;                                                     \
  inner->outgoing.head_buffer.capacity = new_capacity;                                     \
  sbuffer_length_set(inner->outgoing.head_buffer, 0);                                      \
 
#define head_buffer_append_delimiter(buf)                   \
  memcpy(buf, SHTTP_DELIMITER_STR, SHTTP_DELIMITER_LEN)

#define head_buffer_append_crlf(buf)                        \
  memcpy(buf, SHTTP_CRLF_STR, SHTTP_CRLF_LEN)



static inline shttp_res _shttp_response_write_header_copy(shttp_connection_internal_t *inner,
    const char *buf,
    size_t     buf_len) {
  spool_t                              *pool;
  size_t                               new_capacity;
  char                                 *p;

  pool = &inner->pool;
  try_realloc_head_buffer(buf_len, return SHTTP_RES_MEMORY);
mem_copy_begin:
  memcpy(p, buf, buf_len);
  sbuffer_length_add(inner->outgoing.head_buffer, buf_len);
  return SHTTP_RES_OK;
}


static inline shttp_res _shttp_response_write_header_format(shttp_connection_internal_t *inner,
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
  pool = &inner->pool;
  tried = false;

#define return_st1 va_end(args); return SHTTP_RES_MEMORY

retry:
  try_realloc_head_buffer(estimate_length, return_st1);
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
  sbuffer_length_add(inner->outgoing.head_buffer, res);
  va_end(args);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_write_header(shttp_connection_t *conn,
    const char *key,
    size_t     key_len,
    const char *value,
    size_t     value_len) {
  shttp_connection_internal_t          *inner;
  size_t                               estimate_length, new_capacity;
  spool_t                              *pool;
  char                                 *p;

  inner = (shttp_connection_internal_t*)conn->internal;

  if(SHTTP_RES_OK != inner->outgoing.failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == inner->inner.response.head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  assert(0 == inner->outgoing.is_body_end);

  do_predefine_headers(key, key_len, value, value_len);

  if(conn->request.headers.length >= inner->outgoing.headers.capacity) {
    return SHTTP_RES_HEAD_TOO_LARGE;
  }

  pool = conn->pool;
  estimate_length = key_len + SHTTP_DELIMITER_LEN + value_len + SHTTP_CRLF_LEN;
  try_realloc_head_buffer(estimate_length, return SHTTP_RES_MEMORY);

mem_copy_begin:
  conn->request.headers.array[conn->request.headers.length].key.str = p;
  conn->request.headers.array[conn->request.headers.length].key.len = key_len;
  memcpy(p, key, key_len);
  p += key_len;
  head_buffer_append_delimiter(p);
  p += SHTTP_DELIMITER_LEN;

  conn->request.headers.array[conn->request.headers.length].val.str = p;
  conn->request.headers.array[conn->request.headers.length].val.len = value_len;
  memcpy(p, value, value_len);
  p += value_len;
  head_buffer_append_crlf(p);
  p += SHTTP_CRLF_LEN;

  sbuffer_length_add(inner->outgoing.head_buffer, p- (sbuffer_unused_addr(inner->outgoing.head_buffer)));
  conn->request.headers.length += 1;
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_write_header_format(shttp_connection_t *conn,
    const char *key,
    size_t     key_len,
    const char *fmt,
    ...) {
  shttp_connection_internal_t          *inner;
  size_t                               estimate_length, new_capacity;
  int                                  res;
  spool_t                              *pool;
  char                                 *p;
  va_list                              args;
  boolean                              tried;
  shttp_res                            rc;

  inner = (shttp_connection_internal_t*)conn->internal;
  
  if(SHTTP_RES_OK != inner->outgoing.failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == inner->inner.response.head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  assert(0 == inner->outgoing.is_body_end);

  if(conn->request.headers.length >= inner->outgoing.headers.capacity) {
    return SHTTP_RES_HEAD_TOO_LARGE;
  }

  va_start(args, fmt);

  tried = false;
  pool = conn->pool;
  estimate_length = key_len + SHTTP_DELIMITER_LEN + strlen(fmt) + 32 + SHTTP_CRLF_LEN;

retry:
  try_realloc_head_buffer(estimate_length, goto failed);

mem_copy_begin:
  new_capacity = sbuffer_unused_size(inner->outgoing.head_buffer);
  p = sbuffer_unused_addr(inner->outgoing.head_buffer);
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

  do_predefine_headers(key, key_len, p + key_len + SHTTP_DELIMITER_LEN, res);

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
  sbuffer_length_add(inner->outgoing.head_buffer, estimate_length);

  conn->request.headers.array[conn->request.headers.length].key.str = p;
  conn->request.headers.array[conn->request.headers.length].key.len = key_len;
  conn->request.headers.array[conn->request.headers.length].val.str = p + key_len + SHTTP_DELIMITER_LEN;
  conn->request.headers.array[conn->request.headers.length].val.len = res;

  conn->request.headers.length += 1;

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

static shttp_res _shttp_response_send_predefine_headers(shttp_connection_internal_t *inner,
    shttp_connection_t *conn) {
  cstring_t     *status_code;
  int           res;
  shttp_res     rc;
  sbuf_t        buf;

  status_code = shttp_status_code_text(conn->response.status_code);
  if(nil == status_code) {
    return SHTTP_RES_ERROR_STATUS_CODE;
  }

#define _shttp_header_copy(inner, buf, buf_len)                 \
  rc = _shttp_response_write_header_copy(inner, buf, buf_len);  \
  if(SHTTP_RES_OK != rc) {                                      \
    return rc;                                                  \
  }

  //  Connection header
  _shttp_header_copy(inner,
                     "Connection" SHTTP_DELIMITER_STR,
                     10 + SHTTP_DELIMITER_LEN);

  switch(conn->response.close_connection) {
  case SHTTP_CLOSE_CONNECTION_NONE:
    if(0 == http_should_keep_alive(&inner->incomming.parser)) {
      _shttp_header_copy(inner,
                         SHTTP_CONNECTION_CLOSE_STR SHTTP_CRLF_STR,
                         SHTTP_CONNECTION_CLOSE_LEN + SHTTP_CRLF_LEN);
    } else {
      _shttp_header_copy(inner,
                         SHTTP_CONNECTION_KEEP_ALIVE_STR SHTTP_CRLF_STR,
                         SHTTP_CONNECTION_KEEP_ALIVE_LEN + SHTTP_CRLF_LEN);
    }
    break;
  case SHTTP_CLOSE_CONNECTION_TRUE:
    _shttp_header_copy(inner,
                       SHTTP_CONNECTION_KEEP_ALIVE_STR SHTTP_CRLF_STR,
                       SHTTP_CONNECTION_KEEP_ALIVE_LEN + SHTTP_CRLF_LEN);
    break;
  default:
    _shttp_header_copy(inner,
                       SHTTP_CONNECTION_CLOSE_STR SHTTP_CRLF_STR,
                       SHTTP_CONNECTION_CLOSE_LEN + SHTTP_CRLF_LEN);
    break;
  }

  // Content-Type Header
  _shttp_header_copy(inner,
                     "Content-Type" SHTTP_DELIMITER_STR,
                     12 + SHTTP_DELIMITER_LEN);
  _shttp_header_copy(inner,
                     conn->response.content_type.str,
                     conn->response.content_type.len);
  _shttp_header_copy(inner, SHTTP_CRLF_STR, SHTTP_CRLF_LEN);

  // Transfer-Encoding AND Content-Length Header
  if(SHTTP_THUNKED_TRUE == conn->response.chunked) {
    _shttp_header_copy(inner,
                       "Transfer-Encoding" SHTTP_DELIMITER_STR "chunked" SHTTP_CRLF_STR,
                       17 + SHTTP_DELIMITER_LEN + 7 + SHTTP_CRLF_LEN);
  } else if(((size_t)-1) != conn->response.content_length) { // Content-Length Header
    rc = _shttp_response_write_header_format(inner, 28,
         "Content-Length" SHTTP_DELIMITER_STR "%d" SHTTP_CRLF_STR,
         conn->response.content_length);
    if(SHTTP_RES_OK != rc) {
      return rc;
    }
  }
  if(0 == inner->outgoing.head_write_buffers.length) {
    inner->outgoing.head_write_buffers.length = 1;
  }
  
  _shttp_header_copy(inner, SHTTP_CRLF_STR, SHTTP_CRLF_LEN);
  WRITE_TO_UV_BUF(inner, head_write_buffers,
        inner->outgoing.head_buffer.str,
        sbuffer_length(inner->outgoing.head_buffer));
  sbuffer_zero(inner->outgoing.head_buffer);

  res = spool_prepare_alloc(conn->pool, &buf);
  if(SHTTP_RES_OK != res) {
    return res;
  }

  res = snprintf(buf.str, buf.len, "HTTP/%d.%d %d ",
                 conn->response.http_major,
                 conn->response.http_minor,
                 conn->response.status_code);
  if(-1 == res) {
    goto failed;
  }

  if((res + status_code->len + SHTTP_CRLF_LEN) > buf.len) {
    goto failed;
  }
  memcpy(buf.str + res, status_code->str, status_code->len);
  head_buffer_append_crlf(buf.str + res + status_code->len);

  inner->outgoing.head_write_buffers.array[0].base = buf.str;
  inner->outgoing.head_write_buffers.array[0].len = res + status_code->len + SHTTP_CRLF_LEN;
  if(0 == inner->outgoing.head_write_buffers.length) {
    inner->outgoing.head_write_buffers.length = 1;
  }

  spool_commit_alloc(conn->pool, res + status_code->len + SHTTP_CRLF_LEN);
  return SHTTP_RES_OK;
failed:
  spool_rollback_alloc(conn->pool);
  return SHTTP_RES_MEMORY;
}

DLL_VARIABLE shttp_res shttp_response_write(shttp_connection_t *conn,
    const char *data,
    int length,
    shttp_write_cb cb,
    void *cb_data) {
  shttp_res                            rc;
  shttp_connection_internal_t          *inner;
  inner = (shttp_connection_internal_t*)conn->internal;

  inner->inner.response.content_length += length;
  
  if(SHTTP_RES_OK != inner->outgoing.failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }

  WRITE_TO_UV_BUF(inner, body_write_buffers, (char*)data, length);
  if(nil != cb) {
    CALL_AFTER_WRITED(inner, cb, cb_data);
  }
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_end(shttp_connection_t *conn) {
  shttp_res                            rc;
  shttp_connection_internal_t          *inner;
  inner = (shttp_connection_internal_t*)conn->internal;
  
  if(SHTTP_RES_OK != inner->outgoing.failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 != inner->inner.response.head_writed) {
    assert(0 == inner->outgoing.is_body_end);
    rc = _shttp_response_send_predefine_headers(inner, &inner->inner);
    if(SHTTP_RES_OK != rc) {
      ERR("callback: write predefine header failed.");
      inner->outgoing.failed_code = rc;
      return rc;
    }
    inner->inner.response.head_writed = 1;
  }

  
  if(nil != inner->outgoing.content_type.str) {
    spool_free(&inner->pool, inner->outgoing.content_type.str);
    inner->outgoing.content_type.len = 0;
    inner->outgoing.content_type.str = nil;
  }


  assert(0 == inner->outgoing.is_body_end);
  inner->outgoing.is_body_end = 1;
  return SHTTP_RES_OK;
}


void _shttp_response_send_ready_data(shttp_connection_internal_t *conn,
                                     uv_close_cb on_disconnect,
                                     uv_write_cb on_head_writed,
                                     uv_write_cb on_writed) {
  int rc;
  
  if(SHTTP_RES_OK != conn->outgoing.failed_code) {

    conn->outgoing.head_write_buffers.length = 0;
    conn->outgoing.body_write_buffers.length = 0;
    _shttp_response_call_hooks_after_writed(conn, &conn->outgoing);
    _shttp_response_send_error_message_format(conn, on_disconnect, 
      INTERNAL_ERROR_FROMAT, shttp_strerr(conn->outgoing.failed_code));
    return;
  }

  if(0 == conn->outgoing.is_body_end &&
    0 == conn->inner.response.chunked) {
    conn->outgoing.failed_code = SHTTP_RES_BODY_NOT_COMPLETE;
    ERR("callback: chunked must is true while body is not completed.");    
    
    conn->outgoing.head_write_buffers.length = 0;
    conn->outgoing.body_write_buffers.length = 0;
    _shttp_response_call_hooks_after_writed(conn, &conn->outgoing);
    _shttp_response_send_error_message(conn, on_disconnect, 
      BODY_NOT_COMPLETE, strlen(BODY_NOT_COMPLETE));
    return;
  }

  if(1 != conn->inner.response.head_writed) {
    assert(0 == conn->outgoing.is_body_end);
    rc = _shttp_response_send_predefine_headers(conn, &conn->inner);
    if(SHTTP_RES_OK != rc) {
      ERR("callback: write predefine header failed.");
      goto failed;
    }
    conn->inner.response.head_writed = 1;
  }

  if(0 == conn->outgoing.head_write_buffers.length) {
    if(0 == conn->outgoing.body_write_buffers.length) {
      ERR("callback: write buffers is empty.");
      goto failed;
    }
    
    rc = uv_write(&conn->outgoing.write_req,
      (uv_stream_t*)&conn->uv_handle,
      &conn->outgoing.body_write_buffers.array[0],
      (unsigned long)conn->outgoing.body_write_buffers.length,
      on_writed);
  } else {
    rc = uv_write(&conn->outgoing.write_req,
      (uv_stream_t*)&conn->uv_handle,
      &conn->outgoing.head_write_buffers.array[0],
      (unsigned long)conn->outgoing.head_write_buffers.length,
      on_head_writed);
  }
  if(0 == rc) {
    return;
  }
  ERR("write: %s", uv_strerror(rc));
failed:  
  conn->outgoing.head_write_buffers.length = 0;
  conn->outgoing.body_write_buffers.length = 0;
  _shttp_response_call_hooks_after_writed(conn, &conn->outgoing);
  uv_close((uv_handle_t*) &conn->uv_handle, on_disconnect);
  return;
}

#ifdef __cplusplus
}
#endif