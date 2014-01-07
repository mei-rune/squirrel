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

cstring_t* _shttp_status_code_text(int status) {
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