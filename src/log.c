#include "squirrel_config.h"
#include "internal.h"



#ifdef __cplusplus
extern "C" {
#endif

  
void              *shttp_assert_ctx = nil;
shttp_assert_cb_t  shttp_assert_cb = nil;

static void* __shttp_log_ctx = nil;
static shttp_log_cb_t __shttp_log_printf_fn = nil;

void _shttp_log_printf(int severity, const char *fmt, ...) {
  const char *severity_str;
  va_list ap;
  va_start(ap, fmt);

  if (__shttp_log_printf_fn) {
    __shttp_log_printf_fn(__shttp_log_ctx, severity, fmt, ap);
    va_end(ap);
    return;
  }

  switch (severity) {
  case SHTTP_LOG_DEBUG:
    severity_str = "debug";
    break;
  case SHTTP_LOG_WARN:
    severity_str = "warn";
    break;
  case SHTTP_LOG_ERR:
    severity_str = "err";
    break;
  case SHTTP_LOG_CRIT:
    severity_str = "crit";
    break;
  default:
    severity_str = "???";
    break;
  }
  (void)fprintf(stderr, "shttp [%s] ", severity_str);
  (void)vfprintf(stderr, fmt, ap);
  (void)fputs("\r\n", stderr);
  va_end(ap);
}

void shttp_set_log_callback(shttp_log_cb_t cb, void* ctx) {
  __shttp_log_printf_fn = cb;
  __shttp_log_ctx = ctx;
}

void _shttp_log_data(const char* data, size_t len, const char* fmt, ...) {
  va_list arg_ptr;
  va_start(arg_ptr, fmt);

  __shttp_log_printf_fn(__shttp_log_ctx, SHTTP_LOG_DEBUG, fmt, arg_ptr);
  fwrite(data, sizeof(char), len, stdout);
  fprintf(stdout, "\n");

  va_end(arg_ptr);
}


#ifdef __cplusplus
}
#endif