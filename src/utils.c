
#include "squirrel_config.h"
#include "squirrel_string.h"

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

#ifdef __cplusplus
}
#endif