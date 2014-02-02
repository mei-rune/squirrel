
#include "squirrel_config.h"
#include "squirrel_string.h"
#include "squirrel_test.h"



#define ASSERT_NEXT(s, l)                     \
  ASSERT_EQ(0, stoken_next(&ctx, &buf));      \
  ASSERT_EQ(l, buf.len);                      \
  ASSERT_STRNCMP(s, buf.str, buf.len)


TEST(sstring, stoken) {
  stoken_ctx_t ctx;
  cstring_t    buf;


  stoken_init(&ctx, "/a/b/c", -1, "/", 1);
  ASSERT_NEXT("", 0);
  ASSERT_NEXT("a", 1);
  ASSERT_NEXT("b", 1);
  ASSERT_NEXT("c", 1);
  ASSERT_EQ(-1, stoken_next(&ctx, &buf));


  stoken_init(&ctx, "/a/b/c/", -1, "/", 1);
  ASSERT_NEXT("", 0);
  ASSERT_NEXT("a", 1);
  ASSERT_NEXT("b", 1);
  ASSERT_NEXT("c", 1);
  ASSERT_NEXT("", 0);
  ASSERT_EQ(-1, stoken_next(&ctx, &buf));


  stoken_init(&ctx, "a/b/c", -1, "/", 1);
  ASSERT_NEXT("a", 1);
  ASSERT_NEXT("b", 1);
  ASSERT_NEXT("c", 1);
  ASSERT_EQ(-1, stoken_next(&ctx, &buf));


  stoken_init(&ctx, "a/b/c/", -1, "/", 1);
  ASSERT_NEXT("a", 1);
  ASSERT_NEXT("b", 1);
  ASSERT_NEXT("c", 1);
  ASSERT_NEXT("", 0);
  ASSERT_EQ(-1, stoken_next(&ctx, &buf));

  
  stoken_init(&ctx, "a/b//c/", -1, "/", 1);
  ASSERT_NEXT("a", 1);
  ASSERT_NEXT("b", 1);
  ASSERT_NEXT("", 0);
  ASSERT_NEXT("c", 1);
  ASSERT_NEXT("", 0);
  ASSERT_EQ(-1, stoken_next(&ctx, &buf));
}



TEST(sstring, stoken2) {
  stoken_ctx_t ctx;
  cstring_t    buf;

  stoken_init(&ctx, "//a//b//c", -1, "//", 2);
  ASSERT_NEXT("", 0);
  ASSERT_NEXT("a", 1);
  ASSERT_NEXT("b", 1);
  ASSERT_NEXT("c", 1);
  ASSERT_EQ(-1, stoken_next(&ctx, &buf));


  stoken_init(&ctx, "//a//b//c//", -1, "//", 2);
  ASSERT_NEXT("", 0);
  ASSERT_NEXT("a", 1);
  ASSERT_NEXT("b", 1);
  ASSERT_NEXT("c", 1);
  ASSERT_NEXT("", 0);
  ASSERT_EQ(-1, stoken_next(&ctx, &buf));


  stoken_init(&ctx, "a//b//c", -1, "//", 2);
  ASSERT_NEXT("a", 1);
  ASSERT_NEXT("b", 1);
  ASSERT_NEXT("c", 1);
  ASSERT_EQ(-1, stoken_next(&ctx, &buf));


  stoken_init(&ctx, "a//b//c//", -1, "//", 2);
  ASSERT_NEXT("a", 1);
  ASSERT_NEXT("b", 1);
  ASSERT_NEXT("c", 1);
  ASSERT_NEXT("", 0);
  ASSERT_EQ(-1, stoken_next(&ctx, &buf));


  stoken_init(&ctx, "a//b//c/", -1, "//", 2);
  ASSERT_NEXT("a", 1);
  ASSERT_NEXT("b", 1);
  ASSERT_NEXT("c/", 2);
  ASSERT_EQ(-1, stoken_next(&ctx, &buf));


  stoken_init(&ctx, "a//b//c//", strlen("a//b//c//")-1, "//", 2);
  ASSERT_NEXT("a", 1);
  ASSERT_NEXT("b", 1);
  ASSERT_NEXT("c/", 2);
  ASSERT_EQ(-1, stoken_next(&ctx, &buf));
}