
#include "squirrel_config.h"

#include <assert.h>

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

static int _http_parser_on_message_begin(http_parser* inner) {
  shttp_incomming_t* incomming = (shttp_incomming_t*)inner->data;
  incomming->status = shttp_message_begin;
  return 0;
}

static int _http_parser_on_status(http_parser *inner, const char *at, size_t length) {
  //shttp_incomming_t* incomming = (shttp_incomming_t*)inner->data;
  return -1;
}

static int _http_parser_on_url(http_parser *inner, const char *at, size_t length) {
  shttp_incomming_t* incomming = (shttp_incomming_t*)inner->data;

  switch (incomming->status) {
  case shttp_message_begin:
    incomming->status = shttp_message_url;
    incomming->stack[0].str = at;
    incomming->stack[0].len = length;
    break;
  case shttp_message_url:
    assert((incomming->stack[0].str + incomming->stack[0].len) == at);
    incomming->stack[0].len += length;
    break;
  default:
    assert(false);
  }
  return 0;
}

static int _http_parser_on_url_complete(shttp_incomming_t* incomming) {
  struct http_parser_url       parse_url;
  shttp_uri_t                  uri;
  int                          rc;
  cstring_t                    data;


  data.str = incomming->stack[0].str;
  data.len = incomming->stack[0].len;

  rc = http_parser_parse_url(incomming->stack[0].str, incomming->stack[0].len,
                             (HTTP_CONNECT == incomming->parser.method)? 1:0, &parse_url);
  if(0 != rc) {
    ERR("parse_url: style error\n");
    return -1;
  }

  sstring_init(&uri.full, data.str, data.len);
  sstring_init(&uri.schema,
               data.str + parse_url.field_data[UF_SCHEMA].off,
               parse_url.field_data[UF_SCHEMA].len);

  sstring_init(&uri.host,
               data.str + parse_url.field_data[UF_HOST].off,
               parse_url.field_data[UF_HOST].len);

  sstring_init(&uri.port_s,
               data.str + parse_url.field_data[UF_PORT].off,
               parse_url.field_data[UF_PORT].len);

  uri.port_i = parse_url.port;

  sstring_init(&uri.path,
               data.str + parse_url.field_data[UF_PATH].off,
               parse_url.field_data[UF_PATH].len);

  sstring_init(&uri.query,
               data.str + parse_url.field_data[UF_QUERY].off,
               parse_url.field_data[UF_QUERY].len);

  sstring_init(&uri.fragment,
               data.str + parse_url.field_data[UF_FRAGMENT].off,
               parse_url.field_data[UF_FRAGMENT].len);

  sstring_init(&uri.user_info,
               data.str + parse_url.field_data[UF_USERINFO].off,
               parse_url.field_data[UF_USERINFO].len);

  return incomming->callbacks->on_message_begin(incomming->conn,
         incomming->parser.http_major,
         incomming->parser.http_minor,
         incomming->parser.method,
         &uri);
}

static int _http_parser_on_header_field(http_parser *inner, const char *at, size_t length) {
  int                          rc;
  shttp_incomming_t               *incomming;

  incomming = (shttp_incomming_t*)inner->data;
  switch(incomming->status) {
  case shttp_message_url:
    rc = _http_parser_on_url_complete(incomming);
    if(0 != rc) {
      return rc;
    }
    incomming->status = shttp_message_field;
    incomming->stack[0].str = at;
    incomming->stack[0].len = length;
    break;
  case shttp_message_field:
    assert((incomming->stack[0].str + incomming->stack[0].len) == at);
    incomming->stack[0].len += length;
    break;
  case shttp_message_value:
    rc = incomming->callbacks->on_header(incomming->conn, &incomming->stack[0], &incomming->stack[1]);
    if(0 != rc) {
      return rc;
    }
    incomming->status = shttp_message_field;
    incomming->stack[0].str = at;
    incomming->stack[0].len = length;
    break;
  default:
    assert(false);
  }
  return 0;
}

static int _http_parser_on_header_value(http_parser *inner, const char *at, size_t length) {
  shttp_incomming_t               *incomming;

  incomming = (shttp_incomming_t*)inner->data;
  switch(incomming->status) {
  case shttp_message_value:
    assert((incomming->stack[1].str + incomming->stack[1].len) == at);
    incomming->stack[1].len += length;
    break;
  case shttp_message_field:
    incomming->status = shttp_message_value;
    incomming->stack[1].str = at;
    incomming->stack[1].len = length;
    break;
  default:
    assert(false);
  }
  return 0;
}

static int _http_parser_on_headers_complete(http_parser* inner) {
  int                          rc;
  shttp_incomming_t               *incomming;

  incomming = (shttp_incomming_t*)inner->data;
  switch(incomming->status) {
  case shttp_message_value:
    rc = incomming->callbacks->on_header(incomming->conn, &incomming->stack[0], &incomming->stack[1]);
    if(0 != rc) {
      return rc;
    }
    incomming->status = shttp_message_body;
    break;
  default:
    assert(false);
  }
  return 0;
}

static int _http_parser_on_body(http_parser *inner, const char *at, size_t length) {
  shttp_incomming_t               *incomming;
  incomming = (shttp_incomming_t*)inner->data;
  assert(incomming->status == shttp_message_body);
  return incomming->callbacks->on_body(incomming->conn, at, length);
}

static int _http_parser_on_message_complete(http_parser* inner) {
  //shttp_incomming_t* incomming = (shttp_incomming_t*)inner->data;
  return 0;
}


void _shttp_parser_init(struct http_parser_settings *parser_settings) {
  parser_settings->on_message_begin = _http_parser_on_message_begin;
  parser_settings->on_url = _http_parser_on_url;
  parser_settings->on_status = _http_parser_on_status;
  parser_settings->on_header_field = _http_parser_on_header_field;
  parser_settings->on_header_value = _http_parser_on_header_value;
  parser_settings->on_headers_complete = _http_parser_on_headers_complete;
  parser_settings->on_body = _http_parser_on_body;
  parser_settings->on_message_complete = _http_parser_on_message_complete;
}

#ifdef __cplusplus
};
#endif