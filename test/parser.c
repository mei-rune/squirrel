#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "http_parser.h"


static int _http_request_on_message_begin(http_parser* parser) {
  printf("on_message_begin\n");
  return 0;
}

static int _http_request_on_url(http_parser *parser, const char *at, size_t length) {
  size_t  i;
  if(0 == length) {
    //parser->http_major
    printf("on_url http_major=%d, http_minor=%d, method=%d end\n", parser->http_major, parser->http_minor, parser->method);
  } else {
    printf("on_url ");
    for(i = 0; i < length; i ++) {
      putchar(at[i]);
    }
    printf(" \n");
  }
  printf("on_url http_major=%d, http_minor=%d, method=%d end\n", parser->http_major, parser->http_minor, parser->method);
  return 0;
}

static int _http_request_on_header_field(http_parser *parser, const char *at, size_t length) {
  size_t  i;

  //printf("on_url http_major=%d, http_minor=%d, method=%d end\n", parser->http_major, parser->http_minor, parser->method);

  printf("on_header_field ");
  for( i = 0; i < length; i ++) {
    putchar(at[i]);
  }
  printf(" \n");
  return 0;
}

static int _http_request_on_header_value(http_parser *parser, const char *at, size_t length) {
  size_t  i;
  printf("on_header_value ");
  for( i = 0; i < length; i ++) {
    putchar(at[i]);
  }
  printf(" \n");
  return 0;
}

static int _http_request_on_headers_complete(http_parser* parser) {
  printf("on_headers_complete \n");
  return 0;
}

static int _http_request_on_message_complete(http_parser *parser) {
  printf("on_message_complete \n");
  return 0;
}
static int _http_request_on_body(http_parser *parser, const char *at, size_t length) {
  size_t  i;
  printf("on_body");
  for( i = 0; i < length; i ++) {
    putchar(at[i]);
  }
  printf(" \n");
  return 0;
}
static int _http_request_on_status(http_parser *parser, const char *at, size_t length) {
  size_t  i;
  printf("on_body");
  for( i = 0; i < length; i ++) {
    putchar(at[i]);
  }
  printf(" \n");
  return 0;
}

int _http_request_init(struct http_parser_settings *parser_settings) {
  parser_settings->on_message_begin = _http_request_on_message_begin;
  parser_settings->on_url = _http_request_on_url;
  parser_settings->on_status = _http_request_on_status;
  parser_settings->on_header_field = _http_request_on_header_field;
  parser_settings->on_header_value = _http_request_on_header_value;
  parser_settings->on_headers_complete = _http_request_on_headers_complete;
  parser_settings->on_body = _http_request_on_body;
  parser_settings->on_message_complete = _http_request_on_message_complete;
}

int main() {
  //size_t  i;
  struct http_parser parser;
  struct http_parser_settings parser_settings;
  const char* raw = "GET /favicon.ico HTTP/1.1\r\n"
                    "Host: 0.0.0.0=5000\r\n"
                    "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
                    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                    "Accept-Language: en-us,en;q=0.5\r\n"
                    "Accept-Encoding: gzip,deflate\r\n"
                    "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
                    "Keep-Alive: 300\r\n"
                    "Connection: keep-alive\r\n"
                    "\r\n";
  _http_request_init(&parser_settings);
  http_parser_init(&parser, HTTP_REQUEST);
  http_parser_execute(&parser, &parser_settings, raw, strlen(raw));
  //for( i =0; i < strlen(raw); i ++ ) {
  //  int n = http_parser_execute(&parser, &parser_settings, &raw[i], 1);
  //  if(1 != n) {
  //    printf("size error.\n");
  //  }
  //}
  return 0;
}