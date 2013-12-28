#include "squirrel_config.h"

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif


#define RESPONSE_HEADER_SIZE 1024

  /*****************************************************************************
   * HTTP Server Response API
   ****************************************************************************/
#define http_connection_header_value(req) \
    (char*)(http_should_keep_alive(&(req)->client->parser) ? "close" : "keep-alive")

  static void _http_request__write_callback(uv_write_t* write_req, int status) {
    // get the callback object, which is in front of the write_req
    shttp_write_cb_t *callback_obj = (shttp_write_cb_t*)(((uint8_t*)write_req) -
                                     sizeof(shttp_write_cb_t));
    // Call our callback
    if (callback_obj->cb != NULL) {
      callback_obj->cb(callback_obj->data);
    }
    // Free up the memory
    sl_free(callback_obj);
  }

  static inline
  int _http_request__write(shttp_request_t *req,
                           uv_buf_t *buffers,
                           int count,
                           shttp_write_cb cb,
                           void *cb_data) {
    // Malloc memory for our extra callback stuff and the uv_write_t
    shttp_write_cb_t *callback_obj;

    callback_obj = (shttp_write_cb_t *)sl_malloc(sizeof(shttp_write_cb_t) +
                   sizeof(uv_write_t));

    // Setup our own special callback
    callback_obj->cb = cb;
    callback_obj->data = cb_data;

    // Do the write
    return uv_write(
             (uv_write_t*)(((uint8_t*)callback_obj) + sizeof(shttp_write_cb_t)), // uv_write_t ptr
             (uv_stream_t*)&req->client.uv_handle, // handle to write to
             buffers, // uv_buf_t
             count, // # of buffers to write
             _http_request__write_callback
           );
  }

#define HTTP_RESPONSE_HEADERS \
    "HTTP/1.1 %d %s\r\n" \
    "Connection: %s\r\n" \
    "Content-Type: %s\r\n" \
    "Content-Length: %d\r\n" \
    "%s\r\n"

  int http_send_response_string(shttp_request_t *req,
                                         int status,
                                         const char *extra_headers,
                                         const char *content_type,
                                         const char *content,
                                         const uint32_t content_length,
                                         shttp_write_cb cb,
                                         void *cb_data) {
    uv_buf_t b;
    b.base = (char*)content;
    b.len = content_length;
    return http_send_response_buffers(req,
           status,
           extra_headers,
           content_type,
           (http_buf_t*)&b,
           1,
           cb,
           cb_data);
  }


  int http_send_response_buffers(shttp_request_t *req,
                                          int status,
                                          const char *extra_headers,
                                          const char *content_type,
                                          http_buf_t *content_buffers,
                                          int content_buffers_count,
                                          shttp_write_cb cb,
                                          void *cb_data) {

    uv_buf_t* buffers;
    uv_buf_t header;
    char *headers_buffer;
    int content_length = 0;
    int length;
    int i;


    buffers = (uv_buf_t*) content_buffers;
    headers_buffer = (char *)my_malloc(RESPONSE_HEADER_SIZE);

    for (i=0; i < content_buffers_count; i++) {
      content_length += buffers[i].len;
    }

    length = snprintf(
               headers_buffer,
               RESPONSE_HEADER_SIZE,
               HTTP_RESPONSE_HEADERS,
               status,
               http_status_code_text(status),
               http_connection_header_value(req),
               content_type,
               content_length,
               extra_headers);

    // Write headers
    header.base = headers_buffer;
    header.len = length;

	 // Call free on the headers buffer
    _http_request__write(req, &header, 1, my_free_without_check, headers_buffer);
	 // Call callback sent for the content
    _http_request__write(req, buffers, content_buffers_count, cb, cb_data);

    return 0; // TODO what should I return here?
  }


#define HTTP_RESPONSE_HEADERS_CHUNKED \
    "HTTP/1.1 %d %s\r\n" \
    "Connection: %s\r\n" \
    "Content-Type: %s\r\n" \
    "Transfer-Encoding: chunked\r\n" \
    "%s\r\n"

  int http_send_chunked_start(shttp_request_t *req,
                                          int status,
                                          const char *extra_headers,
                                          const char *content_type) {
    uv_buf_t header;
    char *headers_buffer;

    headers_buffer = (char *)my_malloc(RESPONSE_HEADER_SIZE);
    int length = snprintf(
                   headers_buffer,
                   RESPONSE_HEADER_SIZE,
                   HTTP_RESPONSE_HEADERS_CHUNKED,
                   status,
                   http_status_code_text(status),
                   http_connection_header_value(req),
                   content_type,
                   extra_headers);

    // Write headers
    header.base = headers_buffer;
    header.len = length;

	 // Call free on the headers buffer
    _http_request__write(req, &header, 1, my_free_without_check, headers_buffer);
    return 0;
  }


  int http_send_chunked_write(shttp_request_t *req,
                                          const char *data,
                                          int data_length,
                                          shttp_write_cb cb,
                                          void *cb_data) {
    uv_buf_t b;
    int length;
    char *headers_buffer;

    headers_buffer = (char*)my_malloc(32);
    length = snprintf(headers_buffer, 32, "%X\r\n", data_length);

    // Write headers
    b.base = headers_buffer;
    b.len = length;
	 // Call free on the headers buffer
    _http_request__write(req, &b, 1, my_free_without_check, headers_buffer);

    // write content
    b.base = (char*)data;
    b.len = data_length;

	 // Call free on the headers buffer
    _http_request__write(req, &b, 1, cb, cb_data);
    return 0;
  }


  int http_send_chunked_end(shttp_request_t *req) {
    return 0;
  }


#ifdef __cplusplus
}
#endif