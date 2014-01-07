#ifndef _SHTTP_SLAB_H_INCLUDED_
#define _SHTTP_SLAB_H_INCLUDED_ 1

#include "squirrel_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct shttp_slab_page_s  shttp_slab_page_t;

struct shttp_slab_page_s {
  uintptr_t           slab;
  shttp_slab_page_t   *next;
  uintptr_t           prev;
};

typedef struct {
  size_t              min_size;
  size_t              min_shift;

  shttp_slab_page_t   *pages;
  shttp_slab_page_t   free;

  uint8_t             *start;
  uint8_t             *end;

  void                *addr;
} shttp_slab_pool_t;

typedef struct {
  size_t      pool_size, used_size, used_pct;
  size_t      pages, free_page;
  size_t      p_small, p_exact, p_big, p_page; /* 四种slab占用的page数 */
  size_t      b_small, b_exact, b_big, b_page; /* 四种slab占用的byte数 */
  size_t      max_free_pages;          /* 最大的连续可用page数 */
} shttp_slab_stat_t;

void shttp_slab_init(shttp_slab_pool_t *pool);
void *shttp_slab_alloc(shttp_slab_pool_t *pool, size_t size);
void shttp_slab_free(shttp_slab_pool_t *pool, void *p);

void shttp_slab_dummy_init(shttp_slab_pool_t *pool);
void shttp_slab_stat(shttp_slab_pool_t *pool, shttp_slab_stat_t *stat);

#ifdef __cplusplus
};
#endif

#endif /* _SHTTP_SLAB_H_INCLUDED_ */
