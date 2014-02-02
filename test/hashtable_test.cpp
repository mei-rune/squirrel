#include "squirrel_hashtable.h"
#include "squirrel_string.h"
#include "squirrel_pool.h"
#include "squirrel_test.h"

#define hash_int(x) x
#define cmp_int(x,y) (x - y)
#define del_int(k, v)

HASH_DECLARE(EMPTY, int2int, int, int);
HASH_DEFINE(EMPTY, int2int, int, int, hash_int, cmp_int, del_int);

typedef struct int_thunk_s {
  int x;
  int y;
  struct int_thunk_s* aa;
} int_thunk_t;

#define int_table_get_ref(t) t
#define del_int_table(k, v)
#define int_table_get_key(t) t->x
#define int_table_get_value(t) t->y
#define int_table_get_next(t) t->aa
#define int_table_set_key(t, v) t->x = v
#define int_table_set_value(t,v) t->y = v
#define int_table_set_next(t,v) t->aa = v


HASH_DECLARE_WITH_THUNK(EMPTY, int_table, int_thunk_t, int, int);
HASH_DEFINE_WITH_THUNK(EMPTY, int_table
                       , int_thunk_t
                       , int_thunk_t
                       , int
                       , int
                       , int_table_get_ref
                       , int_table_get_key
                       , int_table_get_value
                       , int_table_get_next
                       , int_table_set_key
                       , int_table_set_value
                       , int_table_set_next
                       , hash_int
                       , cmp_int
                       , del_int_table) ;


TEST(hashtable, simple) {
  int2int_t h;
  int2int_init(&h, 3);
  int2int_put(&h, 1,1);
  ASSERT_EQ(1, int2int_get(&h, 1, 0));
  int2int_del(&h, 1);
  ASSERT_EQ(0, int2int_get(&h, 1, 0));

  int2int_destroy(&h);
}


TEST(hashtable, simple2) {
  int_table_t h;
  int_table_init(&h, 3);
  int_table_put(&h, 1,1);
  ASSERT_EQ(1, int_table_get(&h, 1, 0));
  int_table_del(&h, 1);
  ASSERT_EQ(0, int_table_get(&h, 1, 0));

  int_table_destroy(&h);
}


static int malloc_count = 0;

static inline void t_kv_free(void* s) {
  free(s);
  malloc_count --;
}

static inline char* make_str(const char* s) {
  malloc_count ++;
  return strdup(s);
}

#define kv_get_key(x)   ((x)->name)
#define kv_del_kv(k, v) t_kv_free(v->name); t_kv_free(v->value); t_kv_free(v)
typedef struct kv_s {
  char * name;
  char * value;
} kv_t;

HASH_DECLARE_WITH_ENTRY(EMPTY, kvs, const char*, kv_t*);
HASH_DEFINE_WITH_ENTRY(EMPTY, kvs, const char*, kv_t*, kv_get_key, strhash, strcmp, kv_del_kv);



static inline kv_t *make_kv(const char* k, const char* v) {
  kv_t *kv;
  malloc_count ++;
  kv = (kv_t*)malloc(sizeof(kv_t));

  kv->name = make_str(k);
  kv->value = make_str(v);
  return kv;
}

#define PUT_KV(k, v) kvs_put(&h, k, make_kv(k, v))

#define ASSERT_KV(k, v)                       \
  kv = kvs_get(&h, k, nil);                   \
  ASSERT_NE(nil, kv);                         \
  ASSERT_STREQ(k, kv->name);                  \
  ASSERT_STREQ(v, kv->value)

TEST(hashtable, simple_entry) {
  kv_t     *kv;
  kvs_t     h;

  malloc_count = 0;
  kvs_init(&h, 3);
  PUT_KV("a1", "b1");
  PUT_KV("a2", "b2");
  PUT_KV("a3", "b3");
  PUT_KV("a4", "b4");
  PUT_KV("a5", "b5");
  PUT_KV("a6", "b6");
  PUT_KV("a7", "b7");

  ASSERT_KV("a1", "b1");
  ASSERT_KV("a2", "b2");
  ASSERT_KV("a3", "b3");
  ASSERT_KV("a4", "b4");
  ASSERT_KV("a5", "b5");
  ASSERT_KV("a6", "b6");
  ASSERT_KV("a7", "b7");
  
  ASSERT_EQ(7, kvs_count(&h));

  kvs_del(&h, "a1");
  kvs_del(&h, "a2");
  kvs_del(&h, "a3");
  kvs_del(&h, "a4");
  kvs_del(&h, "a5");
  kvs_del(&h, "a6");
  kvs_del(&h, "a7");
  
  ASSERT_EQ(nil, kvs_get(&h, "a1", nil));
  ASSERT_EQ(nil, kvs_get(&h, "a2", nil));
  ASSERT_EQ(nil, kvs_get(&h, "a3", nil));
  ASSERT_EQ(nil, kvs_get(&h, "a4", nil));
  ASSERT_EQ(nil, kvs_get(&h, "a5", nil));
  ASSERT_EQ(nil, kvs_get(&h, "a6", nil));
  ASSERT_EQ(nil, kvs_get(&h, "a7", nil));

  ASSERT_EQ(0, kvs_count(&h));

  
  PUT_KV("a1", "b1");
  PUT_KV("a2", "b2");
  PUT_KV("a3", "b3");
  PUT_KV("a4", "b4");
  PUT_KV("a5", "b5");
  PUT_KV("a6", "b6");
  PUT_KV("a7", "b7");

  kvs_destroy(&h);

  ASSERT_EQ(0, malloc_count);
}
