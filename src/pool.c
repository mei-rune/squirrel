#include "squirrel_config.h"
#include <assert.h>
#include "squirrel_pool.h"
#include "internal.h"


#define SPOOL_MAGIC ((uint16_t)0xabcd)
#define SPOOL_COOKIE ((uint32_t)0xfdfd)


#define shttp_memzero(p, size)        memset(p, 0, size)

#ifdef __cplusplus
extern "C" {
#endif

#define PTR_OFFSET                       SPOOL_BLOCK_HEAD_USED_SIZE
#define BLOCK_TO_PTR(block)              (((char*)block) + PTR_OFFSET)
#define PTR_TO_BLOCK(p)                  ((spool_block_t*)(((char*)p) - PTR_OFFSET))



#ifdef DEBUG
#define spool_junk(p, size)           memset(p, 0xA5, size)
#define spool_set_cookie(p)           *((uint32_t*)(p)) = SPOOL_COOKIE
#define spool_set_used_size(b, size)  (b)->used_size = size
#define spool_cookie_addr(b)          (((char*)b) + SPOOL_BLOCK_HEAD_USED_SIZE + (b)->used_size)
#define assert_cookie(b)              if(nil != cookie_cb) {                                 \
      if(SPOOL_COOKIE != *(uint32_t*) spool_cookie_addr(b)) {                                \
        cookie_cb(PTR_TO_BLOCK(b));                                                          \
      }                                                                                      \
    } else {                                                                                 \
      assert(SPOOL_COOKIE == *(uint32_t*) spool_cookie_addr(b));                             \
    }
#else
#define spool_junk(p, size)
#define spool_set_cookie(p)
#define spool_set_used_size(b, size)
#define assert_cookie(b)
#endif

#ifdef DEBUG
typedef void (*cookie_cb_t)(void*);
cookie_cb_t cookie_cb;
#endif

DLL_VARIABLE void spool_init(spool_t *pool, char* p, size_t capacity) {
  int i;

  if (0 == shttp_pagesize) {
    os_init();

    if (0 == shttp_pagesize) {
      ERR("Please first call os_init() or os_init() failed.");
      return;
    }
  }

  for(i =0; i < SPOOL_SLOTS; i ++) {
    TAILQ_INIT(&pool->free_slots[i]);
  }
  TAILQ_INIT(&pool->all);
  pool->addr = p;
  pool->start = p;
  pool->capacity = capacity;
  pool->transacting = 0;
}

DLL_VARIABLE void *spool_malloc(spool_t *pool, size_t size) {
  size_t           i, s, slot;
  spool_block_t    *block;
  char             *end;

  if(0 != pool->transacting) {
    return nil;
  }
  if(size >= pool->capacity) {
    return nil;
  }
  if(size >= UINT32_MAX) {
    return nil;
  }


  /*
   * 根据size，计算其对应哪个slot；
   * 假设最小obj为8字节，最大obj为2048字节，则slab分9个规模，分别为
   * 8 16 32 64 128 256 512 1024 2048 4096  8192 other
   * 0  1  2  3   4   5   6    7    8    9    10    11
   */
  if(size > 8192) {
    slot = SPOOL_SLOTS - 1;
    i = SPOOL_SLOTS -1;
    TAILQ_FOREACH(block, &pool->free_slots[i], free_next) {
      if( ((uint32_t)size) <= block->capacity) {
        goto block_lookup_end;
      }
    }
    block = nil;
    goto block_lookup_end;
  }

  if (size > 8) {
    slot = 1;
    for (s = size - 1; s >>= 1; slot++) {
      /* void */
    }
    slot -= SPOOL_MIN_SHIFT;
  } else {
    size = SPOOL_MIN_SIZE;
    slot = 0;
  }

  block = nil;
  for(i = slot; i < SPOOL_SLOTS; i ++) {
    if(!TAILQ_EMPTY(&pool->free_slots[i])) {
      block = TAILQ_FIRST(&pool->free_slots[i]);
      break;
    }
  }

block_lookup_end:
  if (nil == block) {
    end = pool->start + spool_alloc_size(size);
    if(end > (pool->addr + pool->capacity)) {
      return nil;
    }
    block = (spool_block_t*)pool->start;
    pool->start = end;

    memset(block, 0, sizeof(spool_block_t));
    block->magic = SPOOL_MAGIC;
    block->capacity = (uint32_t)((end - (char*)block) - SPOOL_BLOCK_USED_SIZE);
    TAILQ_INSERT_TAIL(&pool->all, block, all_next);
  } else {
    TAILQ_REMOVE(&pool->free_slots[i], block, free_next);
  }

  block->used = 1;
  spool_set_used_size(block, size);
  spool_junk(BLOCK_TO_PTR(block), size);
  spool_set_cookie(BLOCK_TO_PTR(block) + size);
  return BLOCK_TO_PTR(block);
}


#define calc_slot(var, size)                                 \
  if((size) > 8192) {                                        \
    var = SPOOL_SLOTS - 1;                                   \
  } else if ((size) > 8) {                                   \
    var = 1;                                                 \
    for (s = (size) - 1; s >>= 1; slot++) {                  \
      /* void */                                             \
    }                                                        \
    var -= SPOOL_MIN_SHIFT;                                  \
  } else {                                                   \
    var = 0;                                                 \
  }



static inline void *_spool_try_realloc(spool_t *pool, spool_block_t *block, char* p, size_t size) {
  size_t           s, new_capacity, slot;
  spool_block_t    *next;

  next = TAILQ_NEXT(block, all_next);
  if(nil == next) {
    new_capacity = spool_alloc_size(size) - SPOOL_BLOCK_USED_SIZE;
    pool->start += (new_capacity - block->capacity);
    block->capacity = new_capacity;

    spool_set_used_size(block, size);
    spool_set_cookie(BLOCK_TO_PTR(block) + size);
    return p;
  }

  if(0 != next->used) {
    return nil;
  }

  new_capacity = (BLOCK_TO_PTR(next) - BLOCK_TO_PTR(block)) + next->capacity;
  if(size > new_capacity) {
    return nil;
  }

  calc_slot(slot, next->capacity);
  TAILQ_REMOVE(&pool->free_slots[slot], next, free_next);
  TAILQ_REMOVE(&pool->all, next, all_next);
  block->capacity = new_capacity;

  spool_set_used_size(block, size);
  spool_set_cookie(p + size);
  return p;
}

DLL_VARIABLE void *spool_try_realloc(spool_t *pool, void* p, size_t size) {
  spool_block_t    *block;

  block = PTR_TO_BLOCK(p);
  assert(SPOOL_MAGIC == block->magic);

  if(size <= block->capacity) {
    spool_set_used_size(block, size);
    spool_set_cookie(BLOCK_TO_PTR(block) + size);
    return p;
  }

  if(0 != pool->transacting) {
    return nil;
  }

  return _spool_try_realloc(pool, block, (char*)p, size);
}

DLL_VARIABLE void *spool_realloc(spool_t *pool, void* p, size_t size) {
  spool_block_t    *block;
  void             *new_ptr;

  block = PTR_TO_BLOCK(p);
  assert(SPOOL_MAGIC == block->magic);

  if(size <= block->capacity) {
    spool_set_used_size(block, size);
    spool_set_cookie(BLOCK_TO_PTR(block) + size);
    return p;
  }
  if(0 != pool->transacting) {
    return nil;
  }

  new_ptr = _spool_try_realloc(pool, block, (char*)p, size);
  if(nil != new_ptr) {
    return new_ptr;
  }

  new_ptr = spool_malloc(pool, size);
  if(nil == new_ptr) {
    return nil;
  }
  memmove(new_ptr, p, block->capacity);
  spool_free(pool, p);
  return new_ptr;
}

DLL_VARIABLE void spool_free(spool_t *pool, void *p) {
  size_t           s, slot;
  spool_block_t    *block, *next,*prev;

  block = PTR_TO_BLOCK(p);
  assert(SPOOL_MAGIC == block->magic);
  assert_cookie(block);

  for(;;) {
    next = TAILQ_NEXT(block, all_next);
    if(nil == next || 0 != next->used) {
      break;
    }
    calc_slot(slot, next->capacity);
    TAILQ_REMOVE(&pool->free_slots[slot], next, free_next);
    TAILQ_REMOVE(&pool->all, next, all_next);

    block->capacity = (BLOCK_TO_PTR(next) - BLOCK_TO_PTR(block)) + next->capacity;
  }

  for(;; block = prev) {
    prev = TAILQ_PREV(block, spool_blocks_s, all_next);
    if(nil == prev || 0 != prev->used) {
      break;
    }
    calc_slot(slot, prev->capacity);
    TAILQ_REMOVE(&pool->free_slots[slot], prev, free_next);
    TAILQ_REMOVE(&pool->all, block, all_next);

    prev->capacity = (BLOCK_TO_PTR(block) - BLOCK_TO_PTR(prev)) + block->capacity;
  }

  if(0 == pool->transacting && nil == next) {
    TAILQ_REMOVE(&pool->all, block, all_next);

    if(nil == prev) {
#ifdef DEBUG
      assert(TAILQ_EMPTY(&pool->all));
      for(s =0; s < SPOOL_SLOTS; s ++) {
        assert(TAILQ_EMPTY(&pool->free_slots[s]));
      }
      assert(pool->addr == (char*)block);
#endif
      pool->start = pool->addr;
    } else {
      pool->start = (char*)block;
    }
    return;
  }

  calc_slot(slot, block->capacity);

  block->used = 0;
  TAILQ_INSERT_TAIL(&pool->free_slots[slot], block, free_next);
}

DLL_VARIABLE int spool_prepare_alloc(spool_t *pool, sbuf_t *buf) {
  if(0 != pool->transacting) {
    return -11;
  }
  pool->transacting = 1;
  buf->str =BLOCK_TO_PTR( pool->start);
  buf->len = pool->capacity - (pool->start - pool->addr) - SPOOL_BLOCK_USED_SIZE;
  return 0;
}

DLL_VARIABLE void spool_rollback_alloc(spool_t *pool) {
  pool->transacting = 0;
}

DLL_VARIABLE void* spool_commit_alloc(spool_t *pool, size_t size) {
  char          *end;
  spool_block_t *block;

  assert(0 != pool->transacting);
  pool->transacting = 0;

  end = pool->start + spool_alloc_size(size);
  if(end > (pool->addr + pool->capacity)) {
    return nil;
  }

  block = (spool_block_t*)pool->start;
  pool->start = end;

  memset(block, 0, sizeof(spool_block_t));
  block->magic = SPOOL_MAGIC;
  block->used = 1;
  block->capacity = (uint32_t)((end - (char*)block) - SPOOL_BLOCK_USED_SIZE);
  spool_set_used_size(block, size);
  spool_set_cookie(BLOCK_TO_PTR(block) + size);
  TAILQ_INSERT_TAIL(&pool->all, block, all_next);
  return BLOCK_TO_PTR(block);
}

DLL_VARIABLE void spool_stat(spool_t *pool) {
}

#ifdef __cplusplus
};
#endif
