
#include "squirrel_config.h"
#include "squirrel_test.h"
#include "squirrel_link.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TestCase {
  char* name;
  struct _TestCase* _next;
  void (__cdecl *func)(out_fn_t fn);
} TestCase;

TestCase g_unittestlist = {0, 0 };
size_t name_max_len = 0;

DLL_VARIABLE int ADD_RUN_TEST(const char* nm, void (__cdecl *func)(out_fn_t fn)) {
  TestCase* tc = (TestCase*)sl_malloc(sizeof(TestCase));
  tc->name = sl_strdup(nm);
  name_max_len = sl_max(name_max_len, strlen(nm));
  tc->func = func;
  tc->_next = 0;
  SLINK_Push(&g_unittestlist, tc);
  return 0;
}

void output(out_fn_t out_fn, const char* fmt, ...) {
  va_list argList;
  va_start(argList, fmt);
  LOG_VPRINTF(fmt, argList);
  va_end( argList );
}


DLL_VARIABLE int RUN_TEST_BY_NAME(const char* nm, out_fn_t out) {
  TestCase* next;

  next = g_unittestlist._next;
  while(0 != next) {
    if(0 == strcmp(nm, next->name)) {
      (*(next->func))(out);
    }
    next = next->_next;
  }
  return 0;
}

DLL_VARIABLE int RUN_TEST_BY_CATALOG(const char* nm, out_fn_t out) {
  char fmt[512];
  TestCase* next;
  snprintf(fmt, 512, "[%%%us] OK\r\n", name_max_len);

  output(out, "=============== unit test ===============\r\n");
  next = g_unittestlist._next;
  while(0 != next) {
    if(0 == strncmp(nm, next->name, strlen(nm))) {
      (*(next->func))(out);
      output(out, fmt, next->name);
    }
    next = next->_next;
  }

  output(out, "=============== end ===============\r\n");
  return 0;
}

DLL_VARIABLE int RUN_ALL_TESTS(out_fn_t out) {
  char fmt[512];
  TestCase* next;
  snprintf(fmt, 512, "[%%%us] OK\r\n", name_max_len);

  output(out, "=============== unit test ===============\r\n");
  next = g_unittestlist._next;
  while(0 != next) {
    (*(next->func))(out);
    output(out, fmt, next->name);
    next = next->_next;
  }

  output(out, "=============== end ===============\r\n");
  return 0;
}


#ifdef __cplusplus
}
#endif