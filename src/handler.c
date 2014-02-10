#include "squirrel_config.h"
#include "squirrel_http.h"



#ifdef __cplusplus
extern "C" {
#endif

DLL_VARIABLE int shttp_notfound_handler(shttp_context_t* ctx) {
  shttp_response_start(ctx, 404, HTTP_CONTENT_TEXT.str, HTTP_CONTENT_TEXT.len);
  shttp_response_write_nocopy(ctx, "NOT FOUND", 9, nil, nil);
  shttp_response_end(ctx);
  return 0;
}

#ifdef __cplusplus
};
#endif