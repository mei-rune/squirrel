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
  XX(100, "100 Continue"),
  XX(101, "101 Switching Protocols"),
  XX(102, "102 Processing"),
  XX(103, nil)
};

/* Successful 2xx */
static size_t HTTP_STATUS_200_LEN;
static cstring_t HTTP_STATUS_200[] = {
  XX(200, "200 OK"),
  XX(201, "201 Created"),
  XX(202, "202 Accepted"),
  XX(203, "203 Non-Authoritative Information"),
  XX(204, "204 No Content"),
  XX(205, "205 Reset Content"),
  XX(206, "206 Partial Content"),
  XX(207, "207 Multi-Status"),
  XX(208, nil)
};

/* Redirection 3xx */
static size_t HTTP_STATUS_300_LEN;
static cstring_t HTTP_STATUS_300[] = {
  XX(300, "300 Multiple Choices"),
  XX(301, "301 Moved Permanently"),
  XX(302, "302 Moved Temporarily"),
  XX(303, "303 See Other"),
  XX(304, "304 Not Modified"),
  XX(305, "305 Use Proxy"),
  XX(307, "307 Temporary Redirect"),
  XX(308, nil)
};

/* Client Error 4xx */
static size_t HTTP_STATUS_400_LEN;
static cstring_t HTTP_STATUS_400[] = {
  XX(400, "400 Bad Request"),
  XX(401, "401 Unauthorized"),
  XX(402, "402 Payment Required"),
  XX(403, "403 Forbidden"),
  XX(404, "404 Not Found"),
  XX(405, "405 Method Not Allowed"),
  XX(406, "406 Not Acceptable"),
  XX(407, "407 Proxy Authentication Required"),
  XX(408, "408 Request Time-out"),
  XX(409, "409 Conflict"),
  XX(410, "410 Gone"),
  XX(411, "411 Length Required"),
  XX(412, "412 Precondition Failed"),
  XX(413, "413 Request Entity Too Large"),
  XX(414, "414 Request-URI Too Large"),
  XX(415, "415 Unsupported Media Type"),
  XX(416, "416 Requested Range Not Satisfiable"),
  XX(417, "417 Expectation Failed"),
  XX(418, "418 I'm a teapot"),
  XX(422, "422 Unprocessable Entity"),
  XX(423, "423 Locked"),
  XX(424, "424 Failed Dependency"),
  XX(425, "425 Unordered Collection"),
  XX(426, "426 Upgrade Required"),
  XX(428, "428 Precondition Required"),
  XX(429, "429 Too Many Requests"),
  XX(431, "431 Request Header Fields Too Large"),
  XX(432, nil)
};

/* Server Error 5xx */
static size_t HTTP_STATUS_500_LEN;
static cstring_t HTTP_STATUS_500[] = {
  XX(500, "500 Internal Server Error"),
  XX(501, "501 Not Implemented"),
  XX(502, "502 Bad Gateway"),
  XX(503, "503 Service Unavailable"),
  XX(504, "504 Gateway Time-out"),
  XX(505, "505 HTTP Version Not Supported"),
  XX(506, "506 Variant Also Negotiates"),
  XX(507, "507 Insufficient Storage"),
  XX(509, "509 Bandwidth Limit Exceeded"),
  XX(510, "510 Not Extended"),
  XX(511, "511 Network Authentication Required"),
  XX(512, nil)
};

static cstring_t UNKNOWN_STATUS_CODE = {0, "UNKNOWN STATUS CODE"};

#undef XX
/**@}*/



void _shttp_status_init() {
  size_t i;

#define XX(num)   for(i =0; nil != HTTP_STATUS_##num##[0].str; i ++) {    \
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




#define WRITE_TO_UV_BUF_ARRAY(inner_conn, buf, buf_len) if(inner_conn->outgoing.write_buffers.capacity <=      \
                        inner_conn->outgoing.write_buffers.length) {                                           \
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
                                            size_t content_type_len){
  shttp_connection_internal_t          *inner;

  inner = (shttp_connection_internal_t*)conn->internal;
  if(1 == inner->inner.response.head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  
  inner->inner.response.status_code = status; 
  copy_content_type(content_type, content_type_len);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_set_chuncked(shttp_connection_t *conn){
  shttp_connection_internal_t          *inner;
  inner = (shttp_connection_internal_t*)conn->internal;
  if(1 == inner->inner.response.head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
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

static void _shttp_c_free (shttp_connection_t *conn, void *data) {
  free(data);
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
    if(estimate_length < sbuffer_unused_size(inner->outgoing.head_buffer)) {               \
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
    if(0 == inner->outgoing.write_buffers.length) {                                        \
       /* this is for first line of response.*/                                            \
      inner->outgoing.write_buffers.length = 1;                                            \
    }                                                                                      \
    if(0 == sbuffer_length(inner->outgoing.head_buffer)) {                                 \
      spool_free(pool, inner->outgoing.head_buffer.str);                                   \
    } else {                                                                               \
      WRITE_TO_UV_BUF_ARRAY(inner, inner->outgoing.head_buffer.str,                        \
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

#define head_snprintf(...)                                  \
  res = snprintf(__VA_ARGS__);                              \
  if(0 > res) {                                             \
    rc = SHTTP_RES_MEMORY;                                  \
    goto failed;                                            \
  }                                                         \
  length += res


#define head_memcpy(dst, src, src_len)                      \
  if(src_len > (buf.len - length)) {                        \
    rc = SHTTP_RES_MEMORY;                                  \
    goto failed;                                            \
  }                                                         \
  memcpy(dst, src, src_len);                                \
  length += src_len


#define head_buffer_append_delimiter(buf)                   \
  memcpy(buf, SHTTP_DELIMITER_STR, SHTTP_DELIMITER_LEN)

#define head_buffer_append_crlf(buf)                        \
  memcpy(buf, SHTTP_CRLF_STR, SHTTP_CRLF_LEN)



static inline shttp_res shttp_response_write_header_copy(shttp_connection_internal_t *inner,
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
                                    char       **new_buf,
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
  
  if(1 == inner->inner.response.head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }

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
  head_buffer_append_crlf(p + key_len);
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
  
  if(1 == inner->inner.response.head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  
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

static shttp_res _shttp_response_send_headers(shttp_connection_internal_t *inner,
                                              shttp_connection_t *conn) {
  size_t        estimate_length, length;
  cstring_t     *status_code;
  sstring_t     buf;
  int           is_alloced, res;
  shttp_res     rc;
  char          *p;

  status_code = shttp_status_code_text(conn->response.status_code);
  if(nil == status_code) {
    return SHTTP_RES_ERROR_STATUS_CODE;
  }
  
  estimate_length = 10 + status_code->len + 10 + SHTTP_CRLF_LEN +                  // HTTP/1.1 %d %s\r\n
    10 + SHTTP_DELIMITER_LEN + SHTTP_CONNECTION_MAX_LEN + SHTTP_CRLF_LEN +         // Connection: %s\r\n
    12 + SHTTP_DELIMITER_LEN + conn->response.content_type.len + SHTTP_CRLF_LEN +  // Content-Type: %s\r\n
    14 + SHTTP_DELIMITER_LEN + 10 + SHTTP_CRLF_LEN;                                // Content-Length: %d\r\n

  if(nil != inner->outgoing.head_buffer.str) {
    if(SHTTP_CRLF_LEN < sbuffer_unused_size(inner->outgoing.head_buffer)) {
      memcpy(sbuffer_unused_addr(inner->outgoing.head_buffer), SHTTP_CRLF_STR, SHTTP_CRLF_LEN);
      sbuffer_length_add(inner->outgoing.head_buffer, SHTTP_CRLF_LEN);
    } else {
      p = (char*)spool_try_realloc(conn->pool, 
        inner->outgoing.head_buffer.str,
        inner->outgoing.head_buffer.len + SHTTP_CRLF_LEN);
      if(nil == p) {
        //asdfasdfasfsadf
      } else {
        inner->outgoing.head_buffer.capacity += SHTTP_CRLF_LEN;
        memcpy(sbuffer_unused_addr(inner->outgoing.head_buffer), SHTTP_CRLF_STR, SHTTP_CRLF_LEN);
        sbuffer_length_add(inner->outgoing.head_buffer, SHTTP_CRLF_LEN);
      }
    }
    buf.str = shttp_align_ptr(sbuffer_unused_addr(inner->outgoing.head_buffer), sizeof(void*));
    buf.len = inner->outgoing.head_buffer.capacity - (buf.str - inner->outgoing.head_buffer.str);
    if(estimate_length >  buf.len) {
      rc = spool_prepare_alloc(conn->pool, &buf);
      if(SHTTP_RES_OK == rc) {
        return rc;
      }
      is_alloced = 1;
    } else {
      is_alloced = 0;
    }
    WRITE_TO_UV_BUF_ARRAY(inner, inner->outgoing.head_buffer.str, 
      sbuffer_length(inner->outgoing.head_buffer));
    inner->outgoing.head_buffer.str = nil;
    sbuffer_length_set(inner->outgoing.head_buffer, 0);
  } else {
    rc = spool_prepare_alloc(conn->pool, &buf);
    if(SHTTP_RES_OK == rc) {
      return rc;
    }
    is_alloced = 1;
  }
  length = 0;
    


  head_snprintf(buf.str, buf.len, "HTTP/%d.%d %d ", 
    conn->response.http_major,
    conn->response.http_minor,
    conn->response.status_code);

  head_memcpy(buf.str + length, status_code->str, status_code->len);

  //  Connection header
  head_memcpy(buf.str + length, SHTTP_CRLF_STR "Connection" SHTTP_DELIMITER_STR,
    SHTTP_CRLF_LEN + 10 + SHTTP_DELIMITER_LEN);
  switch(conn->response.close_connection) {
  case SHTTP_CLOSE_CONNECTION_NONE:
    if(0 == http_should_keep_alive(&inner->incomming.parser)) {
      head_memcpy(buf.str + length, SHTTP_CONNECTION_CLOSE_STR, SHTTP_CONNECTION_CLOSE_LEN);
    } else {
      head_memcpy(buf.str + length, SHTTP_CONNECTION_KEEP_ALIVE_STR, SHTTP_CONNECTION_KEEP_ALIVE_LEN);
    }
    break;
  case SHTTP_CLOSE_CONNECTION_TRUE:
    head_memcpy(buf.str + length, SHTTP_CONNECTION_KEEP_ALIVE_STR, SHTTP_CONNECTION_KEEP_ALIVE_LEN);
    break;
  default:
    head_memcpy(buf.str + length, SHTTP_CONNECTION_CLOSE_STR, SHTTP_CONNECTION_CLOSE_LEN);
    break;
  }

  // Content-Type Header
  head_memcpy(buf.str + length, SHTTP_CRLF_STR "Content-Type" SHTTP_DELIMITER_STR,
    SHTTP_CRLF_LEN + 12 + SHTTP_DELIMITER_LEN);
  head_memcpy(buf.str + length, conn->response.content_type.str, conn->response.content_type.len);
  
  // 
  if(SHTTP_THUNKED_TRUE == conn->response.chunked) {
    head_memcpy(buf.str + length, SHTTP_CRLF_STR "Transfer-Encoding" SHTTP_DELIMITER_STR "chunked" SHTTP_CRLF_STR,
      SHTTP_CRLF_LEN + 17 + SHTTP_DELIMITER_LEN + 7 + SHTTP_CRLF_LEN);
  } else if(((size_t)-1) != conn->response.content_length) { // Content-Length Header
    head_snprintf(buf.str + length, buf.len - length, 
      SHTTP_CRLF_STR "Content-Length" SHTTP_DELIMITER_STR "%d" SHTTP_CRLF_STR,
      conn->response.content_length);
  } else {
    head_memcpy(buf.str + length, SHTTP_CRLF_STR, SHTTP_CRLF_LEN);
  }

  if(1 == is_alloced) {
    if(nil == spool_commit_alloc(conn->pool, length)) {
      return SHTTP_RES_MEMORY;
    }
    CALL_AFTER_WRITED(inner, &shttp_response_pool_free, buf.str);
  }
  assert(nil == inner->outgoing.write_buffers.array[0].base);
  inner->outgoing.write_buffers.array[0].base = buf.str;
  inner->outgoing.write_buffers.array[0].len = length;
  if(0 == inner->outgoing.write_buffers.length) {
    inner->outgoing.write_buffers.length +=1;
  }
  return SHTTP_RES_OK;
failed:
  if(1 == is_alloced) {
    spool_rollback_alloc(conn->pool);
  }
  return rc;
}

DLL_VARIABLE shttp_res shttp_response_write(shttp_connection_t *conn,
                               const char *data,
                               int length,
                               shttp_write_cb cb,
                               void *cb_data) {
  shttp_res                            rc;
  shttp_connection_internal_t          *inner;
  inner = (shttp_connection_internal_t*)conn->internal;
  
  if(1 != inner->inner.response.head_writed) {
    rc = _shttp_response_send_headers(inner, conn);
    if(SHTTP_RES_OK != rc) {
      return rc;
    }
    inner->inner.response.head_writed = 1;
  }
  
  WRITE_TO_UV_BUF_ARRAY(inner, (char*)data, length);
  if(nil != cb) {
    CALL_AFTER_WRITED(inner, cb, cb_data);
  }
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_end(shttp_connection_t *conn) {
  shttp_res                            rc;
  shttp_connection_internal_t          *inner;
  inner = (shttp_connection_internal_t*)conn->internal;
  
  if(1 != inner->inner.response.head_writed) {
    rc = _shttp_response_send_headers(inner, conn);
    if(SHTTP_RES_OK != rc) {
      return rc;
    }
    inner->inner.response.head_writed = 1;
  }
  return SHTTP_RES_OK;
}

//  /*****************************************************************************
//   * HTTP Server Response API
//   ****************************************************************************/
//#define shttp_connection_header_value(req) \
//    (char*)(http_should_keep_alive(&(req)->client->parser) ? "close" : "keep-alive")
//
//  static void _http_request__write_callback(uv_write_t* write_req, int status) {
//    // get the callback object, which is in front of the write_req
//    shttp_write_cb_t *callback_obj = (shttp_write_cb_t*)(((uint8_t*)write_req) -
//                                     sizeof(shttp_write_cb_t));
//    // Call our callback
//    if (callback_obj->cb != NULL) {
//      callback_obj->cb(callback_obj->data);
//    }
//    // Free up the memory
//    sl_free(callback_obj);
//  }
//
//  static inline
//  int _shttp_write(shttp_connection_internal_t *conn,
//                           uv_buf_t *buffers,
//                           int count,
//                           shttp_write_cb cb,
//                           void *cb_data) {
//    // Malloc memory for our extra callback stuff and the uv_write_t
//    uv_write_t
//    shttp_write_cb_t *callback_obj;
//
//    callback_obj = (shttp_write_cb_t *)sl_malloc(sizeof(shttp_write_cb_t) +
//                   sizeof(uv_write_t));
//
//    // Setup our own special callback
//    callback_obj->cb = cb;
//    callback_obj->data = cb_data;
//
//    // Do the write
//    return uv_write(
//             (uv_write_t*)(((uint8_t*)callback_obj) + sizeof(shttp_write_cb_t)), // uv_write_t ptr
//             (uv_stream_t*)&conn->uv_handle, // handle to write to
//             buffers, // uv_buf_t
//             count, // # of buffers to write
//             _http_request__write_callback
//           );
//  }
//
//#define HTTP_RESPONSE_HEADERS \
//    "HTTP/1.1 %d %s\r\n" \
//    "Connection: %s\r\n" \
//    "Content-Type: %s\r\n" \
//    "Content-Length: %d\r\n" \
//    "%s\r\n"
//
//  int shttp_send_response_string(shttp_connection_t *conn,
//                                         int status,
//                                         const char *extra_headers,
//                                         const char *content_type,
//                                         const char *content,
//                                         const uint32_t content_length,
//                                         shttp_write_cb cb,
//                                         void *cb_data) {
//    cstring_t b;
//    b.str = (const char*)content;
//    b.len = content_length;
//    return http_send_response_buffers(conn,
//           status,
//           extra_headers,
//           content_type,
//           (cstring_t*)&b,
//           1,
//           cb,
//           cb_data);
//  }
//
//
//  int shttp_send_response_buffers(shttp_connection_t *conn,
//                                          int status,
//                                          const char *extra_headers,
//                                          const char *content_type,
//                                          cstring_t  *content_buffers,
//                                          int content_buffers_count,
//                                          shttp_write_cb cb,
//                                          void *cb_data) {
//    uv_buf_t                    *buffers;
//    uv_buf_t                    header;
//    char                        *headers_buffer;
//    int                         content_length = 0;
//    int                         length;
//    int                         i;
//    shttp_connection_internal_t *conn_internal;
//
//    conn_internal = (shttp_connection_internal_t*)conn->internal;
//    assert(conn_internal->outgoing.start == 0);
//    assert(conn_internal->outgoing.end == 0);
//
//    buffers = (uv_buf_t*) conn_internal->outgoing.base;
//    conn_internal->outgoing.start = (content_buffers_count+1)  * sizeof(uv_buf_t);
//    headers_buffer = conn_internal->outgoing.base + conn_internal->outgoing.start;
//
//    for (i=0; i < content_buffers_count; i++) {
//      buffers[i+1].base = (char*)content_buffers[i].str;
//      buffers[i+1].len = content_buffers[i].len;
//      content_length += buffers[i].len;
//    }
//
//    length = snprintf(headers_buffer,
//               conn_internal->outgoing.capacity - conn_internal->outgoing.start,
//               HTTP_RESPONSE_HEADERS,
//               status,
//               _shttp_status_code_text(status)->str,
//               (0 == http_should_keep_alive(&conn_internal->incomming.parser))?"close": "keep-alive",
//               content_type,
//               content_length,
//               extra_headers);
//
//    // Write headers
//    buffers[0].base = headers_buffer;
//    buffers[0].len = length;
//
//   // Call callback sent for the content
//    _shttp_write(conn_internal, buffers, content_buffers_count + 1, cb, cb_data);
//
//    return 0; // TODO what should I return here?
//  }
//
//
//#define HTTP_RESPONSE_HEADERS_CHUNKED \
//    "HTTP/1.1 %d %s\r\n" \
//    "Connection: %s\r\n" \
//    "Content-Type: %s\r\n" \
//    "Transfer-Encoding: chunked\r\n" \
//    "%s\r\n"
//
//  int shttp_send_chunked_start(shttp_connection_t *conn,
//                                          int status,
//                                          const char *extra_headers,
//                                          const char *content_type) {
//    uv_buf_t header;
//    char *headers_buffer;
//    shttp_connection_internal_t *conn_internal;
//
//    conn_internal = (shttp_connection_internal_t*)conn->internal;
//
//    headers_buffer = (char *)sl_malloc(RESPONSE_HEADER_SIZE);
//    int length = snprintf(
//                   headers_buffer,
//                   RESPONSE_HEADER_SIZE,
//                   HTTP_RESPONSE_HEADERS_CHUNKED,
//                   status,
//                   _shttp_status_code_text(status)->str,
//                  (0 == http_should_keep_alive(&conn_internal->parser.inner))?"close": "keep-alive",
//                   content_type,
//                   extra_headers);
//
//    // Write headers
//    header.base = headers_buffer;
//    header.len = length;
//
//   // Call free on the headers buffer
//    _http_request__write(req, &header, 1, my_free_without_check, headers_buffer);
//    return 0;
//  }
//
//
//  int shttp_send_chunked_write(shttp_connection_t *conn,
//                                          const char *data,
//                                          int data_length,
//                                          shttp_write_cb cb,
//                                          void *cb_data) {
//    uv_buf_t b;
//    int length;
//    char *headers_buffer;
//
//    headers_buffer = (char*)sl_malloc(32);
//    length = snprintf(headers_buffer, 32, "%X\r\n", data_length);
//
//    // Write headers
//    b.base = headers_buffer;
//    b.len = length;
//    // Call free on the headers buffer
//    _http_request__write(req, &b, 1, my_free_without_check, headers_buffer);
//
//    // write content
//    b.base = (char*)data;
//    b.len = data_length;
//
//    // Call free on the headers buffer
//    _http_request__write(req, &b, 1, cb, cb_data);
//    return 0;
//  }
//
//
//  int http_send_chunked_end(shttp_connection_t *conn) {
//    return 0;
//  }
//
//
#ifdef __cplusplus
}
#endif