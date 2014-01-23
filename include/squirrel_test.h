
#ifndef _squirrel_testting_h_
#define _squirrel_testting_h_ 1

#include "squirrel_config.h"

// Include files
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "squirrel_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHECK(condition)                                                         \
  do {                                                                           \
    if (!(condition)) {                                                          \
      LOG_PRINTF_2("Check failed: " #condition " at %s(%d)\n"                    \
                               , __FILE__, __LINE__);                            \
      abort();                                                                   \
      exit(1);                                                                   \
    }                                                                            \
  } while (0)


#define CHECK_MESSAGE(condition, message)                                         \
  do {                                                                            \
    if (!(condition)) {                                                           \
      LOG_PRINTF_2("Check failed: " #condition ": " message " at %s(%d)\n"        \
                               , __FILE__, __LINE__);                             \
      abort();                                                                    \
      exit(1);                                                                    \
    }                                                                             \
  } while (0)


#define PCHECK(condition)                                                         \
  do {                                                                            \
    if (!(condition)) {                                                           \
     LOG_PRINTF_3("Check failed: " #condition ": %s at %s(%d)\n"                  \
                         , strerror(errno), __FILE__, __LINE__);                  \
      abort();                                                                    \
      exit(1);                                                                    \
    }                                                                             \
  } while (0)

#define CHECK_OP(op, val1, val2)                                                  \
  do {                                                                            \
    if (!((val1) op (val2))) {                                                    \
      LOG_PRINTF_2("Check failed: " #val1 #op #val2 " at %s, %d\n"                \
                        , __FILE__, __LINE__ );                                   \
      abort();                                                                    \
      exit(1);                                                                    \
    }                                                                             \
  } while (0)

#define CHECK_EQ(val1, val2) CHECK_OP(==, val1, val2)
#define CHECK_NE(val1, val2) CHECK_OP(!=, val1, val2)
#define CHECK_LE(val1, val2) CHECK_OP(<=, val1, val2)
#define CHECK_LT(val1, val2) CHECK_OP(< , val1, val2)
#define CHECK_GE(val1, val2) CHECK_OP(>=, val1, val2)
#define CHECK_GT(val1, val2) CHECK_OP(> , val1, val2)

#define ASSERT_EQ(val1, val2) CHECK_EQ(val1, val2)
#define ASSERT_NE(val1, val2) CHECK_NE(val1, val2)
#define ASSERT_LE(val1, val2) CHECK_LE(val1, val2)
#define ASSERT_LT(val1, val2) CHECK_LT(val1, val2)
#define ASSERT_GE(val1, val2) CHECK_GE(val1, val2)
#define ASSERT_GT(val1, val2) CHECK_GT(val1, val2)


#define ASSERT_TRUE(cond)      CHECK(cond)
#define ASSERT_FALSE(cond)     CHECK(!(cond))
#define ASSERT_STREQ(a, b)    CHECK((a == b) || ( a !=0 && b !=0 && strcmp(a, b) == 0))

#define CHECK_ERR(invocation)  PCHECK((invocation) != -1)


#define LOG_VPRINTF(fmt, argList) do {                                \
    if(0 != out_fn)                                                   \
    {                                                                 \
      char buf[600];                                                  \
      size_t len = vsnprintf(buf, sizeof(buf), fmt, argList);         \
      (*out_fn)(buf, len);                                            \
    }                                                                 \
    else                                                              \
    {                                                                 \
      vprintf(fmt, argList);                                          \
    }                                                                 \
} while (0)


#define LOG_PRINTF_0(fmt) do {                                        \
    if(0 != out_fn)                                                   \
        (*out_fn)(fmt, strlen(fmt));                                  \
    else                                                              \
        printf(fmt);                                                  \
} while(0)



#define LOG_PRINTF_1(fmt, arg1) do {                                  \
    if(0 != out_fn)                                                   \
    {                                                                 \
      char buf[600];                                                  \
      size_t len = snprintf(buf, sizeof(buf), fmt, arg1);             \
      (*out_fn)(buf, len);                                            \
    }                                                                 \
    else                                                              \
    {                                                                 \
      printf(fmt, arg1);                                              \
    }                                                                 \
} while (0)

#define LOG_PRINTF_2(fmt, arg1, arg2) do {                            \
    if(0 != out_fn)                                                   \
    {                                                                 \
      char buf[600];                                                  \
      size_t len = snprintf(buf, sizeof(buf), fmt, arg1, arg2);       \
      (*out_fn)(buf, len);                                            \
    }                                                                 \
    else                                                              \
    {                                                                 \
      printf(fmt, arg1, arg2);                                        \
    }                                                                 \
} while (0)

#define LOG_PRINTF_3(fmt, arg1, arg2, arg3) do {                      \
    if(0 != out_fn)                                                   \
    {                                                                 \
      char buf[600];                                                  \
      size_t len = snprintf(buf, sizeof(buf), fmt, arg1, arg2,arg3);  \
      (*out_fn)(buf, len);                                            \
    }                                                                 \
    else                                                              \
    {                                                                 \
      printf(fmt, arg1, arg2,arg3);                                   \
    }                                                                 \
} while (0)

#define LOG_PRINTF_4(fmt, arg1, arg2, arg3, arg4) do {                \
    if(0 != out_fn)                                                   \
    {                                                                 \
      char buf[600];                                                  \
      size_t len = snprintf(buf, sizeof(buf), fmt                     \
                               , arg1, arg2, arg3, arg4);             \
      (*out_fn)(buf, len);                                            \
    }                                                                 \
    else                                                              \
    {                                                                 \
      printf(fmt, arg1, arg2, arg3, arg4);                            \
    }                                                                 \
} while (0)




typedef void (__cdecl *out_fn_t)(const char* buf, size_t len);

#define UNITTEST_TO_STR(x) #x

#ifdef _DEBUG

#define TEST(a, b)                                                    \
void __cdecl test_##a##_##b##_run(out_fn_t out_fn);                   \
int test_##a##_##b##_var = ADD_RUN_TEST(UNITTEST_TO_STR(a##_##b)      \
                 , &test_##a##_##b##_run);                            \
void __cdecl test_##a##_##b##_run(out_fn_t out_fn)


DLL_VARIABLE int ADD_RUN_TEST(const char* nm, void (__cdecl *func)(out_fn_t fn));
DLL_VARIABLE int RUN_ALL_TESTS(out_fn_t out);
DLL_VARIABLE int RUN_TEST_BY_NAME(const char* nm, out_fn_t out);
DLL_VARIABLE int RUN_TEST_BY_CATALOG(const char* nm, out_fn_t out);
#else

#define TEST(a, b)  void __cdecl test_##a##_##b##_run(out_fn_t out_fn)
#define ADD_RUN_TEST(nm, func)
#define RUN_ALL_TESTS(out)
#define RUN_TEST_BY_NAME(nm, out);

#endif

#ifdef __cplusplus
}
#endif

#endif // _testting_h_
