#include "squirrel_config.h"
#include "squirrel_http.h"



#ifdef __cplusplus
extern "C" {
#endif
  
DLL_VARIABLE int shttp_notfound_handler(shttp_connection_t* conn) {
  shttp_response_start(conn, 404, HTTP_CONTENT_TEXT.str, HTTP_CONTENT_TEXT.len);
  shttp_response_write(conn, "NOT FOUND", 9, nil, nil);
  shttp_response_end(conn);
  return 0;
}

#ifdef __cplusplus
};
#endif