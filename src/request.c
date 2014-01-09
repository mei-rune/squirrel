
#include "squirrel_config.h"

#include <assert.h>

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

int _shttp_connection_on_message_begin(shttp_connection_internal_t*);
int _shttp_connection_on_body (shttp_connection_internal_t*, const char *at, size_t length);
int _shttp_connection_on_message_complete (shttp_connection_internal_t*);

static int _http_request_on_message_begin(http_parser* inner) {
  shttp_incomming_t* incomming = (shttp_incomming_t*)inner->data;
  incomming->status = shttp_message_begin;
  incomming->head_reading = 0;
  return _shttp_connection_on_message_begin(incomming->conn);
}

static int _http_request_on_status(http_parser *inner, const char *at, size_t length) {
  shttp_incomming_t* incomming = (shttp_incomming_t*)inner->data;
  incomming->status = shttp_message_status;
  return 0;
}

static int _http_request_on_url(http_parser *inner, const char *at, size_t length) {
  shttp_incomming_t* incomming = (shttp_incomming_t*)inner->data;
  switch (incomming->status) {
  case shttp_message_begin:
    incomming->status = shttp_message_url;
    incomming->url.str = at;
    incomming->url.len = length;
    break;
  case shttp_message_url:
    assert((incomming->url.str + incomming->url.len) == at);
    incomming->url.len += length;
    break;
  default:
    ERR("parse error: url status is unknown(%d).", incomming->status);
    assert(false);
    return -1;
  }
  return 0;
}

static int _parse_url(shttp_incomming_t* incomming) {
  struct http_parser_url       parse_url;
  int                          rc;
  cstring_t                    data;


  data.str = incomming->url.str;
  data.len = incomming->url.len;

  rc = http_parser_parse_url(incomming->url.str, incomming->url.len,
                             (HTTP_CONNECT == incomming->parser.method)? 1:0, &parse_url);
  if(0 != rc) {
    ERR("parse_url: style error\n");
    return -1;
  }

#define http_uri incomming->conn->inner.request.uri

  sstring_init(&http_uri.full, data.str, data.len);
  sstring_init(&http_uri.schema,
               data.str + parse_url.field_data[UF_SCHEMA].off,
               parse_url.field_data[UF_SCHEMA].len);

  sstring_init(&http_uri.host,
               data.str + parse_url.field_data[UF_HOST].off,
               parse_url.field_data[UF_HOST].len);

  sstring_init(&http_uri.port_s,
               data.str + parse_url.field_data[UF_PORT].off,
               parse_url.field_data[UF_PORT].len);

  http_uri.port_i = parse_url.port;

  sstring_init(&http_uri.path,
               data.str + parse_url.field_data[UF_PATH].off,
               parse_url.field_data[UF_PATH].len);

  sstring_init(&http_uri.query,
               data.str + parse_url.field_data[UF_QUERY].off,
               parse_url.field_data[UF_QUERY].len);

  sstring_init(&http_uri.fragment,
               data.str + parse_url.field_data[UF_FRAGMENT].off,
               parse_url.field_data[UF_FRAGMENT].len);

  sstring_init(&http_uri.user_info,
               data.str + parse_url.field_data[UF_USERINFO].off,
               parse_url.field_data[UF_USERINFO].len);
  return 0;
  //return incomming->callbacks->on_message_begin(incomming->conn,
  //       incomming->parser.http_major,
  //       incomming->parser.http_minor,
  //       incomming->parser.method,
  //       &uri);
}

static int _http_request_on_header_field(http_parser *inner, const char *at, size_t length) {
  shttp_incomming_t            *incomming;

#define current_header incomming->headers.array[incomming->headers.length]

  incomming = (shttp_incomming_t*)inner->data;
  switch(incomming->status) {
  case shttp_message_url:
    incomming->status = shttp_message_field;
    current_header.key.str = at;
    current_header.key.len = length;
    break;
  case shttp_message_field:
    assert((current_header.key.str + current_header.key.len) == at);
    current_header.key.len += length;
    break;
  case shttp_message_value:
    incomming->status = shttp_message_field;
    incomming->headers.length +=1;
    if(incomming->headers.capacity >= incomming->headers.length) {
      ERR("parse error: header length too large.");
      return -1;
    }

    current_header.key.str = at;
    current_header.key.len = length;
    break;
  default:
    ERR("parse error: head field status is unknown(%d).", incomming->status);
    assert(false);
    return -1;
  }
  return 0;
}

static int _http_request_on_header_value(http_parser *inner, const char *at, size_t length) {
  shttp_incomming_t               *incomming;
  
#define current_header incomming->headers.array[incomming->headers.length]

  incomming = (shttp_incomming_t*)inner->data;
  switch(incomming->status) {
  case shttp_message_value:
    assert((current_header.val.str + current_header.val.len) == at);
    current_header.val.len += length;
    break;
  case shttp_message_field:
    incomming->status = shttp_message_value;
    current_header.val.str = at;
    current_header.val.len = length;
    break;
  default:
    ERR("parse error: head value status is unknown(%d).", incomming->status);
    assert(false);
    return -1;
  }
  return 0;
}

static int _http_request_on_headers_complete(http_parser* inner) {
  shttp_incomming_t            *incomming;

  incomming = (shttp_incomming_t*)inner->data;
  incomming->status = shttp_message_body;
  incomming->head_reading = 1;
  incomming->headers_bytes_count = incomming->parser.nread;
  incomming->conn->inner.request.headers.array = incomming->headers.array;
  incomming->conn->inner.request.headers.length = incomming->headers.length;
  incomming->conn->inner.request.http_major = inner->http_major;
  incomming->conn->inner.request.http_minor = inner->http_minor;
  incomming->conn->inner.request.method = inner->method;
  incomming->conn->inner.request.content_length = inner->content_length;
  //       incomming->parser.method,

  return _parse_url(incomming);
}

static int _http_request_on_body(http_parser *inner, const char *at, size_t length) {
  shttp_incomming_t               *incomming;
  incomming = (shttp_incomming_t*)inner->data;
  assert(incomming->status == shttp_message_body);
  return _shttp_connection_on_body(incomming->conn, at, length);
}

static int _http_request_on_message_complete(http_parser* inner) {
  shttp_incomming_t* incomming = (shttp_incomming_t*)inner->data;
  return _shttp_connection_on_message_complete(incomming->conn);
}

void _shttp_parser_init(struct http_parser_settings *parser_settings) {
  parser_settings->on_message_begin = _http_request_on_message_begin;
  parser_settings->on_url = _http_request_on_url;
  parser_settings->on_status = _http_request_on_status;
  parser_settings->on_header_field = _http_request_on_header_field;
  parser_settings->on_header_value = _http_request_on_header_value;
  parser_settings->on_headers_complete = _http_request_on_headers_complete;
  parser_settings->on_body = _http_request_on_body;
  parser_settings->on_message_complete = _http_request_on_message_complete;
}

#ifdef __cplusplus
};
#endif