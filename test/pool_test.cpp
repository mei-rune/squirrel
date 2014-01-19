#include "squirrel_config.h"
#include <stdlib.h>
#include "squirrel_pool.h"
#include "squirrel_test.h"

void *spool_test_realloc(spool_t *pool, void* p, size_t size) {
  void *c = spool_realloc(pool, p, size);
  memset(c, '4', size);
  return c;
}
void *spool_test_malloc(spool_t *pool, size_t size) {
  void *p = spool_malloc(pool, size);
  memset(p, '4', size);
  return p;
}
size_t calc_free_slot(size_t size) {
  // 8 16 32 64 128 256 512 1024 2048 4096  8192 other
  // 0  1  2  3   4   5   6    7    8    9    10    11
  if(8 >= size) {
    return 0;
  } else if(16 >= size) {
    return 1;
  } else if(32 >= size) {
    return 2;
  } else if(64 >= size) {
    return 3;
  } else if(128 >= size) {
    return 4;
  } else if(256 >= size) {
    return 5;
  } else if(512 >= size) {
    return 6;
  } else if(1024 >= size) {
    return 7;
  } else if(2048 >= size) {
    return 8;
  } else if(4096 >= size) {
    return 9;
  } else if(8192 >= size) {
    return 10;
  }
  return 11;
}

size_t calc_free_slot2(size_t factor, size_t bytes) {
  return calc_free_slot(factor*shttp_align(bytes + sizeof(spool_block_t)+4, shttp_cacheline_size)-sizeof(spool_block_t)-4);
}

size_t all_queue_size(spool_t *pool) {
  spool_block_t    *block;
  size_t count = 0;

  TAILQ_FOREACH(block, &pool->all, all_next) {
    count ++;
  }
  return count;
}

size_t free_queue_size(spool_t *pool, size_t slot) {
  spool_block_t    *block;
  size_t count = 0;

  TAILQ_FOREACH(block, &pool->free_slots[slot], free_next) {
    count ++;
  }
  return count;
}

void test_pool_once(out_fn_t out_fn, spool_t *pool, size_t bytes) {
  size_t  i, slot;
  void    *m1;

  m1 = spool_test_malloc(pool, bytes);
  slot = calc_free_slot2(1, bytes);
  ASSERT_EQ(1, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  //for(i =0; i < SPOOL_SLOTS; i ++) {
  //  if(slot == i) {
  //    ASSERT_EQ(1, free_queue_size(pool, i));
  //  } else {
  //    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  //  }
  //}
  spool_free(pool, m1);

  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
}

void test_pool_reused(out_fn_t out_fn, spool_t *pool, size_t bytes) {
  size_t  i, slot;
  void    *m1, *m1_1, *m2;

  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }

  slot = calc_free_slot2(1, bytes);
  spool_free(pool, m1);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  m1_1 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }

  ASSERT_EQ(m1, m1_1);


  spool_free(pool, m1_1);
  spool_free(pool, m2);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
}


void test_pool_reused2(out_fn_t out_fn, spool_t *pool, size_t bytes) {
  size_t  i, slot;
  void    *m1, *m1_1, *m2;

  m1 = spool_test_malloc(pool, bytes + 1024);
  m2 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }

  slot = calc_free_slot2(1, bytes + 1024);
  spool_free(pool, m1);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  m1_1 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }

  ASSERT_EQ(m1, m1_1);


  spool_free(pool, m1_1);
  spool_free(pool, m2);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
}


void test_pool_twice(out_fn_t out_fn, spool_t *pool, size_t bytes) {
  size_t  i, slot;
  void    *m1, *m2;


  slot = calc_free_slot2(1, bytes);
  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m1);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  spool_free(pool, m2);

  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }


  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m2);
  ASSERT_EQ(1, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m1);

  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
}

void test_pool3(out_fn_t out_fn, spool_t *pool, size_t bytes) {
  size_t  i, slot;
  void    *m1, *m2, *m3;


  slot = calc_free_slot2(1, bytes);
  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_malloc(pool, bytes);
  m3 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(3, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m1);
  ASSERT_EQ(3, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }

  spool_free(pool, m2);

  slot = calc_free_slot2(2, bytes);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  spool_free(pool, m3);

  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }



  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_malloc(pool, bytes);
  m3 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(3, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m3);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m2);
  ASSERT_EQ(1, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m1);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }


  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_malloc(pool, bytes);
  m3 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(3, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }

  slot = calc_free_slot2(1, bytes);
  spool_free(pool, m2);
  ASSERT_EQ(3, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  slot = calc_free_slot2(2, bytes);
  spool_free(pool, m3);
  ASSERT_EQ(1, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m1);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
}

void test_pool4(out_fn_t out_fn, spool_t *pool, size_t bytes) {
  size_t  i, slot;
  void    *m1, *m2, *m3, *m4;


  slot = calc_free_slot2(1, bytes);
  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_malloc(pool, bytes);
  m3 = spool_test_malloc(pool, bytes);
  m4 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(4, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m1);
  ASSERT_EQ(4, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  slot = calc_free_slot2(2, bytes);
  spool_free(pool, m2);
  ASSERT_EQ(3, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  slot = calc_free_slot2(3, bytes);
  spool_free(pool, m3);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }


  spool_free(pool, m4);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }



  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_malloc(pool, bytes);
  m3 = spool_test_malloc(pool, bytes);
  m4 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(4, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m4);
  ASSERT_EQ(3, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m3);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m2);
  ASSERT_EQ(1, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  spool_free(pool, m1);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }


  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_malloc(pool, bytes);
  m3 = spool_test_malloc(pool, bytes);
  m4 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(4, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }

  slot = calc_free_slot2(1, bytes);
  spool_free(pool, m2);
  ASSERT_EQ(4, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  slot = calc_free_slot2(2, bytes);
  spool_free(pool, m3);
  ASSERT_EQ(3, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  slot = calc_free_slot2(3, bytes);
  spool_free(pool, m1);
  ASSERT_EQ(2, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  spool_free(pool, m4);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
}




void test_pool6(out_fn_t out_fn, spool_t *pool, size_t bytes) {
  size_t  i, slot;
  void    *m1, *m2, *m3, *m4, *m5, *m6;


  slot = calc_free_slot2(1, bytes);
  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_malloc(pool, bytes);
  m3 = spool_test_malloc(pool, bytes);
  m4 = spool_test_malloc(pool, bytes);
  m5 = spool_test_malloc(pool, bytes);
  m6 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(6, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  slot = calc_free_slot2(1, bytes);
  spool_free(pool, m4);
  ASSERT_EQ(6, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  spool_free(pool, m2);
  ASSERT_EQ(6, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(2, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  slot = calc_free_slot2(3, bytes);
  spool_free(pool, m3);
  ASSERT_EQ(4, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  spool_free(pool, m1);
  spool_free(pool, m5);
  spool_free(pool, m6);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
}

void test_pool6_2(out_fn_t out_fn, spool_t *pool, size_t bytes) {
  size_t  i, slot1, slot2;
  void    *m1, *m2, *m3, *m4, *m5, *m6;

  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_malloc(pool, bytes);
  m3 = spool_test_malloc(pool, bytes);
  m4 = spool_test_malloc(pool, bytes);
  m5 = spool_test_malloc(pool, bytes);
  m6 = spool_test_malloc(pool, bytes);
  ASSERT_EQ(6, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
  slot1 = calc_free_slot2(1, bytes);
  spool_free(pool, m4);
  ASSERT_EQ(6, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot1 == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  spool_free(pool, m2);
  ASSERT_EQ(6, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot1 == i) {
      ASSERT_EQ(2, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  slot2 = calc_free_slot2(2, bytes);
  spool_free(pool, m1);
  ASSERT_EQ(5, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot1 == slot2 && slot1 == i) {
      ASSERT_EQ(2, free_queue_size(pool, i));
    } else if(slot1 == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else if(slot2 == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  slot1 = calc_free_slot2(4, bytes);
  spool_free(pool, m3);
  ASSERT_EQ(3, all_queue_size(pool));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    if(slot1 == i) {
      ASSERT_EQ(1, free_queue_size(pool, i));
    } else {
      ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
    }
  }
  spool_free(pool, m5);
  spool_free(pool, m6);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
}

void test_realloc(out_fn_t out_fn, spool_t *pool, size_t bytes) {
  size_t  i;
  void    *m1, *m2;

  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_realloc(pool, m1, bytes+1);

  ASSERT_EQ(m1, m2);

  spool_free(pool, m2);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
}

void test_realloc2(out_fn_t out_fn, spool_t *pool, size_t bytes) {
  size_t  i;
  void    *m1, *m2;

  m1 = spool_test_malloc(pool, bytes);
  m2 = spool_test_realloc(pool, m1, bytes+1024);

  ASSERT_EQ(m1, m2);

  spool_free(pool, m2);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
}
void test_realloc3(out_fn_t out_fn, spool_t *pool, size_t bytes) {
  size_t  i;
  void    *m1, *m2, *m3;

  m1 = spool_test_malloc(pool, bytes);
  m3 = spool_test_malloc(pool, bytes);
  m2 = spool_test_realloc(pool, m1, bytes+1024);

  ASSERT_NE(m1, m2);

  spool_free(pool, m2);
  spool_free(pool, m3);
  ASSERT_EQ(pool->start, pool->addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool->all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool->free_slots[i]));
  }
}



TEST(pool, simple) {
  size_t  i, bytes;
  spool_t pool;
  char    *mem;

#define max_size6 (6 * 17 *1024)
  mem = (char*)malloc(max_size6);
  spool_init(&pool, mem, max_size6);

  for(i =0; i < SPOOL_SLOTS; i ++) {
    bytes = (1 <<(i + SPOOL_MIN_SHIFT))-1;
    test_pool6(out_fn, &pool, bytes);
  }

  for(i =0; i < SPOOL_SLOTS; i ++) {
    bytes = (1 <<(i + SPOOL_MIN_SHIFT))-1;
    test_pool6_2(out_fn, &pool, bytes);
  }

  free(mem);


#define max_size4 (4 * 17 *1024)
  mem = (char*)malloc(max_size4);
  spool_init(&pool, mem, max_size4);


  bytes = (1 <<(2 + SPOOL_MIN_SHIFT))-1;
  test_pool4(out_fn, &pool, bytes);

  for(i =0; i < SPOOL_SLOTS; i ++) {
    bytes = (1 <<(i + SPOOL_MIN_SHIFT))-1;
    test_pool4(out_fn, &pool, bytes);
  }

  free(mem);

#define max_size2 (3 * 17 *1024)
  mem = (char*)malloc(max_size2);
  spool_init(&pool, mem, max_size2);
  for(i =0; i < SPOOL_SLOTS; i ++) {
    bytes = (1 <<(i + SPOOL_MIN_SHIFT))-1;
    test_pool3(out_fn, &pool, bytes);
    test_realloc3(out_fn, &pool, bytes);
  }

  free(mem);

#define max_size1 (2 * 17 *1024)
  mem = (char*)malloc(max_size1);
  spool_init(&pool, mem, max_size1);


  test_pool_twice(out_fn, &pool, 9191);

  for(i =0; i < SPOOL_SLOTS; i ++) {
    bytes = (1 <<(i + SPOOL_MIN_SHIFT))-2;
    test_pool_once(out_fn, &pool, bytes);
    test_pool_reused(out_fn, &pool, bytes);
    test_pool_reused2(out_fn, &pool, bytes);
    test_realloc(out_fn, &pool, bytes);
    test_realloc2(out_fn, &pool, bytes);
  }

  for(i =0; i < SPOOL_SLOTS; i ++) {
    bytes = (1 <<(i + SPOOL_MIN_SHIFT))-1;
    test_pool_twice(out_fn, &pool, bytes);
  }

  free(mem);

}


static int cookie_error = 0;
void on_cookie_error(void* cb) {
  cookie_error ++;
}


TEST(pool, cookie) {
  size_t  i, bytes;
  spool_t pool;
  char    *mem;
  void    *p;

#define max_size_cookie (1024)
  mem = (char*)malloc(max_size_cookie);
  spool_init(&pool, mem, max_size_cookie);

  cookie_error =0;
  cookie_cb = &on_cookie_error;

  p = spool_malloc(&pool, 8);
  memset(p, 'a', 9);
  spool_free(&pool, p);

  ASSERT_EQ(1, cookie_error);
  free(mem);
  cookie_cb = nil;
}


TEST(pool, transaction) {
  size_t  i, bytes;
  spool_t pool;
  char    *mem;
  void    *p;
  sbuf_t  buf;
  int     rc;
  void    *m1, *m2, *m3;

#define max_size_transaction (1024)
  mem = (char*)malloc(max_size_transaction);
  spool_init(&pool, mem, max_size_transaction);

  cookie_error =0;
  cookie_cb = &on_cookie_error;


  p = spool_malloc(&pool, 2);

  rc = spool_prepare_alloc(&pool, &buf);
  ASSERT_EQ(0, rc);
  if(0 != rc) {
    return;
  }
  ASSERT_EQ(nil,  spool_malloc(&pool, 2));
  ASSERT_NE(nil, spool_realloc(&pool, p, 2));
  ASSERT_EQ(nil, spool_realloc(&pool, p, 128));

  memset(buf.str, 'a', 128);
  m1 = spool_commit_alloc(&pool, 128);

  rc = spool_prepare_alloc(&pool, &buf);
  ASSERT_EQ(0, rc);
  if(0 != rc) {
    return;
  }
  memset(buf.str, 'a', 128);
  m2 = spool_commit_alloc(&pool, 128);

  rc = spool_prepare_alloc(&pool, &buf);
  ASSERT_EQ(0, rc);
  if(0 != rc) {
    return;
  }
  memset(buf.str, 'a', 128);
  m3 = spool_commit_alloc(&pool, 128);

  spool_free(&pool, p);
  spool_free(&pool, m1);
  spool_free(&pool, m2);
  spool_free(&pool, m3);
  ASSERT_EQ(pool.start, pool.addr);
  ASSERT_TRUE(TAILQ_EMPTY(&pool.all));
  for(i =0; i < SPOOL_SLOTS; i ++) {
    ASSERT_TRUE(TAILQ_EMPTY(&pool.free_slots[i]));
  }
  free(mem);
}