#include "squirrel_config.h"

#include "squirrel_http.h"
#include "squirrel_test.h"

typedef struct st_kv_s {
  const char    *k;
  const char    *v;
} st_kv_t;
typedef struct st_query_s {
  shttp_kv_t    kvs[1024];
  size_t        len;
} st_query_t;


void on_query(void* ctx, const char* key, size_t key_len, const char* val, size_t val_len) {
  st_query_t  *q;
  q = (st_query_t  *)ctx;
  q->kvs[q->len].key.str = key;
  q->kvs[q->len].key.len = key_len;
  q->kvs[q->len].val.str = val;
  q->kvs[q->len].val.len = val_len;
  q->len ++;
}


void test_query(out_fn_t out_fn, const char *query, const st_kv_t* kvs, size_t count) {
  st_query_t q;
  size_t     i;

  memset(&q, 0, sizeof(st_query_t));
  ASSERT_EQ(0, shttp_parse_query_string(query, strlen(query), &on_query, &q));

  ASSERT_EQ(count, q.len);
  for(i = 0; i < count; i ++) {
    if(nil == kvs[i].k) {
      ASSERT_EQ(0, q.kvs[i].key.len);
      //ASSERT_EQ(nil, q.kvs[i].key.str);
    } else {
      ASSERT_EQ(strlen(kvs[i].k), q.kvs[i].key.len);
      ASSERT_STRNCMP(kvs[i].k, q.kvs[i].key.str, q.kvs[i].key.len);
    }

    if(nil == kvs[i].v) {
      ASSERT_EQ(0, q.kvs[i].val.len);
      //ASSERT_EQ(nil, q.kvs[i].val.str);
    } else {
      ASSERT_EQ(strlen(kvs[i].v), q.kvs[i].val.len);
      ASSERT_STRNCMP(kvs[i].v, q.kvs[i].val.str, q.kvs[i].val.len);
    }
  }
}


st_kv_t t1[] = {{"a", "2"}};
st_kv_t t2[] = {{"a", "2"},{"b", "3"}};
st_kv_t t3[] = {{"a", nil}};
st_kv_t t4[] = {{"a", "2"},{"b", nil}};
st_kv_t t5[] = {{"a", nil},{"b", "2"}};
st_kv_t t6[] = {{"a", nil},{"b", nil}};
st_kv_t t7[] = {{"aa", "1"},{"bb", "2"}};
st_kv_t t8[] = {{"aa", "1"},{"cc", "3"},{"dd", "5"},{"bb", "2"}};

TEST(query_string, simple) {
  test_query(out_fn, "", nil, 0);
  test_query(out_fn, "a=2", t1, 1);
  test_query(out_fn, "a=2&b=3", t2, 2);
  test_query(out_fn, "a", t3, 1);
  test_query(out_fn, "a=", t3, 1);
  test_query(out_fn, "a=2&b", t4, 2);
  test_query(out_fn, "a=2&b=", t4, 2);
  test_query(out_fn, "a&b=2", t5, 2);
  test_query(out_fn, "a=&b=2", t5, 2);

  test_query(out_fn, "a&b", t6, 2);
  test_query(out_fn, "a=&b=", t6, 2);

  test_query(out_fn, "a&b=", t6, 2);
  test_query(out_fn, "a=&b", t6, 2);

  test_query(out_fn, "aa=1&bb=2", t7, 2);
  test_query(out_fn, "aa=1&cc=3&dd=5&bb=2", t8, 4);
}

