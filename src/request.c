
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
  shttp_connection_internal_t* conn;
  conn = (shttp_connection_internal_t*)inner->data;
  conn->status = shttp_message_request_parse_begin;
  conn->head_reading = 0;
  return _shttp_connection_on_message_begin(conn);
}

static int _http_request_on_status(http_parser *inner, const char *at, size_t length) {
  shttp_connection_internal_t* conn;
  
  conn = (shttp_connection_internal_t*)inner->data;
  conn->status = shttp_message_request_parse_status;
  return 0;
}

static int _http_request_on_url(http_parser *inner, const char *at, size_t length) {
  shttp_connection_internal_t* conn;
  
  conn = (shttp_connection_internal_t*)inner->data;
  switch (conn->status) {
  case shttp_message_request_parse_begin:
    conn->status = shttp_message_request_parse_url;
    conn_incomming(conn).url.str = at;
    conn_incomming(conn).url.len = length;
    break;
  case shttp_message_request_parse_url:
    assert((conn_incomming(conn).url.str + conn_incomming(conn).url.len) == at);
    conn_incomming(conn).url.len += length;
    break;
  default:
    ERR("parse error: url status is unknown(%d).", conn->status);
    assert(false);
    return -1;
  }
  return 0;
}

static int _parse_url(shttp_connection_internal_t* conn) {
  struct http_parser_url       parse_url;
  int                          rc;
  cstring_t                    data;


  data.str = conn_incomming(conn).url.str;
  data.len = conn_incomming(conn).url.len;

  rc = http_parser_parse_url(conn_incomming(conn).url.str, conn_incomming(conn).url.len,
                             (HTTP_CONNECT == conn->parser.method)? 1:0, &parse_url);
  if(0 != rc) {
    ERR("parse_url: style error\n");
    return -1;
  }

#define http_uri conn_request(conn).uri

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
  shttp_connection_internal_t* conn;

  conn = (shttp_connection_internal_t*)inner->data;
#define current_header conn_incomming(conn).headers.array[conn_incomming(conn).headers.length]

  switch(conn->status) {
  case shttp_message_request_parse_url:
    conn->status = shttp_message_request_parse_field;
    current_header.key.str = at;
    current_header.key.len = length;
    break;
  case shttp_message_request_parse_field:
    assert((current_header.key.str + current_header.key.len) == at);
    current_header.key.len += length;
    break;
  case shttp_message_request_parse_value:
    conn->status = shttp_message_request_parse_field;
    conn_incomming(conn).headers.length +=1;
    if(conn_incomming(conn).headers.capacity <= conn_incomming(conn).headers.length) {
      ERR("parse error: header length too large.");
      return -1;
    }

    current_header.key.str = at;
    current_header.key.len = length;
    break;
  default:
    ERR("parse error: head field status is unknown(%d).", conn->status);
    assert(false);
    return -1;
  }
  return 0;
}

static int _http_request_on_header_value(http_parser *inner, const char *at, size_t length) {
  shttp_connection_internal_t* conn;

  conn = (shttp_connection_internal_t*)inner->data;

#define current_header conn_incomming(conn).headers.array[conn_incomming(conn).headers.length]

  switch(conn->status) {
  case shttp_message_request_parse_value:
    assert((current_header.val.str + current_header.val.len) == at);
    current_header.val.len += length;
    break;
  case shttp_message_request_parse_field:
    conn->status = shttp_message_request_parse_value;
    current_header.val.str = at;
    current_header.val.len = length;
    break;
  default:
    ERR("parse error: head value status is unknown(%d).", conn->status);
    assert(false);
    return -1;
  }
  return 0;
}

static int _http_request_on_headers_complete(http_parser* inner) {
  shttp_connection_internal_t* conn;

  conn = (shttp_connection_internal_t*)inner->data;
  conn->status = shttp_message_request_parse_body;
  conn->head_reading = 1;
  conn->headers_bytes_count = conn->parser.nread;
  conn_request(conn).headers.array = conn_incomming(conn).headers.array;
  conn_request(conn).headers.length = conn_incomming(conn).headers.length;
  conn_request(conn).http_major = inner->http_major;
  conn_request(conn).http_minor = inner->http_minor;
  conn_request(conn).method = inner->method;
  conn_request(conn).content_length = inner->content_length;

  return _parse_url(conn);
}

static int _http_request_on_body(http_parser *inner, const char *at, size_t length) {
  shttp_connection_internal_t* conn;

  conn = (shttp_connection_internal_t*)inner->data;
  assert(conn->status == shttp_message_request_parse_body);
  return _shttp_connection_on_body(conn, at, length);
}

static int _http_request_on_message_complete(http_parser* inner) {
  shttp_connection_internal_t* conn;

  conn = (shttp_connection_internal_t*)inner->data;
  assert(shttp_message_request_parse_body == conn->status);
  conn->status = shttp_message_request_parse_end;
  return _shttp_connection_on_message_complete(conn);
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