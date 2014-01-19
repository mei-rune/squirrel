#include "squirrel_hashtable.h"
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

#define del_int_table(k, v)
#define int_table_get_key(t) t->x
#define int_table_get_value(t) t->y
#define int_table_get_next(t) t->aa
#define int_table_set_key(t, v) t->x = v
#define int_table_set_value(t,v) t->y = v
#define int_table_set_next(t,v) t->aa = v


HASH_DECLARE2(EMPTY, int_table, int_thunk_t, int, int);
HASH_DEFINE2(EMPTY, int_table
             , int_thunk_t
             , int
             , int
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