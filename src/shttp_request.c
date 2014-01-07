
#include "squirrel_config.h"

#include <assert.h>

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

//static int _http_request_on_message_begin(http_parser* parser) {
//    shttp_connection_internal_t* conn = (shttp_connection_internal_t*)parser->data;
//  conn->data.len =0;
//  conn->message_status = shttp_message_begin;
//    return 0;
//}
//
//static int _http_request_on_status(http_parser *parser, const char *at, size_t length) {
//    //shttp_connection_internal_t* conn = (shttp_connection_internal_t*)parser->data;
//    return -1;
//}
//
//static int _http_request_on_url(http_parser *parser, const char *at, size_t length) {
//    shttp_connection_internal_t* conn = (shttp_connection_internal_t*)parser->data;
//  if(conn->data.capacity - conn->data.len < length) {
//        ERR("parse_url: url is too large.\n");
//    return -1;
//  }
//  memcpy(conn->data.str + conn->data.len, at, length);
//  conn->data.len += length;
//    return 0;
//}
//
//static int _http_request_on_url_complete(http_parser *parser) {
//    shttp_connection_internal_t* conn;
//  struct http_parser_url       parse_url;
//  shttp_uri_t                  uri;
//  int                          rc;
//
//  conn = (shttp_connection_internal_t*)parser->data;
//  rc = http_parser_parse_url(conn->data.str, conn->data.len, ( HTTP_CONNECT == parser->method)? 1:0, &parse_url);
//  if(0 != rc) {
//        ERR("parse_url: style error\n");
//    return -1;
//  }
//
//  sstring_init(&uri.full, conn->data.str, conn->data.len);
//  sstring_init(&uri.schema,
//    conn->data.str + parse_url.field_data[UF_SCHEMA].off,
//    parse_url.field_data[UF_SCHEMA].len);
//
//  sstring_init(&uri.host,
//    conn->data.str + parse_url.field_data[UF_HOST].off,
//    parse_url.field_data[UF_HOST].len);
//
//  sstring_init(&uri.port_s,
//    conn->data.str + parse_url.field_data[UF_PORT].off,
//    parse_url.field_data[UF_PORT].len);
//
//  uri.port_i = parse_url.port;
//
//  sstring_init(&uri.path,
//    conn->data.str + parse_url.field_data[UF_PATH].off,
//    parse_url.field_data[UF_PATH].len);
//
//  sstring_init(&uri.query,
//    conn->data.str + parse_url.field_data[UF_QUERY].off,
//    parse_url.field_data[UF_QUERY].len);
//
//  sstring_init(&uri.fragment,
//    conn->data.str + parse_url.field_data[UF_FRAGMENT].off,
//    parse_url.field_data[UF_FRAGMENT].len);
//
//  sstring_init(&uri.user_info,
//    conn->data.str + parse_url.field_data[UF_USERINFO].off,
//    parse_url.field_data[UF_USERINFO].len);
//
//  return conn->inner.http->settings.callbacks.on_message_begin(&conn->inner,
//    parser->http_major,
//    parser->http_minor,
//    parser->method,
//    &uri);
//}
//
//static int _http_request_on_header_field(http_parser *parser, const char *at, size_t length) {
//  int                          rc;
//    shttp_connection_internal_t* conn;
//  conn = (shttp_connection_internal_t*)parser->data;
//  if(shttp_message_begin == conn->message_status) {
//    conn->message_status = shttp_message_url;
//    rc = _http_request_on_url_complete(parser);
//    if(0 != rc) {
//      return rc;
//    }
//  } else if(shttp_message_begin == conn->message_status) {
//
//  }
//
//    int i = 0;
//
//    if (last_was_value && connection->current_header_key_length > 0)
//    {
//        // Save last read header key/value pair.
//        for (i = 0; connection->current_header_key[i]; i++)
//        {
//            connection->current_header_key[i] = tolower(connection->current_header_key[i]);
//        }
//
//        set_header(connection->request, connection->current_header_key, connection->current_header_value);
//
//        /* Start of a new header */
//        connection->current_header_key_length = 0;
//    }
//    memcpy((char *)&connection->current_header_key[connection->current_header_key_length], at, length);
//    connection->current_header_key_length += length;
//    connection->current_header_key[connection->current_header_key_length] = '\0';
//    last_was_value = 0;
//    return 0;
//}
//
//static int _http_request_on_header_value(http_parser *parser, const char *at, size_t length)
//{
//    shttp_connection_internal_t* connection = (shttp_connection_internal_t*)parser->data;
//
//    if (!last_was_value && connection->current_header_value_length > 0)
//    {
//        /* Start of a new header */
//        connection->current_header_value_length = 0;
//    }
//    memcpy((char *)&connection->current_header_value[connection->current_header_value_length], at, length);
//    connection->current_header_value_length += length;
//    connection->current_header_value[connection->current_header_value_length] = '\0';
//    last_was_value = 1;
//    return 0;
//}
//
//static int _http_request_on_headers_complete(http_parser* parser)
//{
//    shttp_connection_internal_t* connection = (shttp_connection_internal_t*)parser->data;
//    int i = 0;
//
//    if (connection->current_header_key_length > 0)
//    {
//        if (connection->current_header_value_length > 0)
//        {
//            /* Store last header */
//            for (i = 0; connection->current_header_key[i]; i++)
//            {
//                connection->current_header_key[i] = tolower(connection->current_header_key[i]);
//            }
//            set_header(connection->request, connection->current_header_key, connection->current_header_value);
//        }
//        connection->current_header_key[connection->current_header_key_length] = '\0';
//        connection->current_header_value[connection->current_header_value_length] = '\0';
//    }
//    connection->current_header_key_length = 0;
//    connection->current_header_value_length = 0;
//
//    connection->request->http_major = parser->http_major;
//    connection->request->http_minor = parser->http_minor;
//    connection->request->method = parser->method;
//    connection->keep_alive = http_should_keep_alive(parser);
//    connection->request->keep_alive = connection->keep_alive;
//    return 0;
//}
//
//static int _http_request_on_body(http_parser *parser, const char *at, size_t length)
//{
//    shttp_connection_internal_t* connection = (shttp_connection_internal_t*)parser->data;
//    if (length != 0)
//    {
//        connection->request->body->value = realloc(connection->request->body->value, connection->request->body->length + length);
//        memcpy(connection->request->body->value + connection->request->body->length, at, length);
//        connection->request->body->length += length;
//    }
//    return 0;
//}
//
//static int _http_request_on_message_complete(http_parser* parser) {
//    //shttp_connection_internal_t* connection = (shttp_connection_internal_t*)parser->data;
//    return 0;
//}
//
//
//int _http_request_init(struct http_parser_settings *parser_settings) {
//    parser_settings->on_message_begin = _http_request_on_message_begin;
//    parser_settings->on_url = _http_request_on_url;
//    parser_settings->on_status = _http_request_on_status;
//    parser_settings->on_header_field = _http_request_on_header_field;
//    parser_settings->on_header_value = _http_request_on_header_value;
//    parser_settings->on_headers_complete = _http_request_on_headers_complete;
//    parser_settings->on_body = _http_request_on_body;
//    parser_settings->on_message_complete = _http_request_on_message_complete;
//}

#ifdef __cplusplus
};
#endif
