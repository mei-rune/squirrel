#ifndef _SHTTP_POOL_H_INCLUDED_
#define _SHTTP_POOL_H_INCLUDED_ 1

#include "squirrel_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/queue.h>

#include "squirrel_string.h"
#include "squirrel_util.h"

typedef struct spool_block_s {
  uint16_t                    magic;
  uint16_t                    used:1;
  uint16_t                    reserved:15;
#ifdef DEBUG
  uint32_t                    reserved2;
  uint32_t                    used_size;
#endif
  uint32_t                    capacity;
  
  TAILQ_ENTRY(spool_block_s) all_next;
  TAILQ_ENTRY(spool_block_s) free_next;
} spool_block_t;


TAILQ_HEAD(spool_blocks_s, spool_block_s);
typedef struct spool_blocks_s spool_blocks_t;


#define SPOOL_BLOCK_HEAD_USED_SIZE       sizeof(spool_block_t)
#define SPOOL_BLOCK_END_USED_SIZE        4
#define SPOOL_BLOCK_USED_SIZE            (SPOOL_BLOCK_HEAD_USED_SIZE + SPOOL_BLOCK_END_USED_SIZE)
#define spool_alloc_size(size)           shttp_align(size + SPOOL_BLOCK_HEAD_USED_SIZE + SPOOL_BLOCK_END_USED_SIZE, shttp_cacheline_size)
#define spool_excepted(s)                (spool_alloc_size(s) - SPOOL_BLOCK_USED_SIZE)

#define SPOOL_SLOTS       12
#define SPOOL_MIN_SHIFT   3
#define SPOOL_MIN_SIZE    8

typedef struct spool_s {
  spool_blocks_t      free_slots[SPOOL_SLOTS];
  spool_blocks_t      all;

  char                *addr;
  size_t              capacity;

  char                *start;

  
  uint16_t                    transacting:1;
  uint16_t                    reserved1:15;
  uint32_t                    reserved2;
} spool_t;

DLL_VARIABLE void spool_init(spool_t *pool, char* p, size_t capacity);
DLL_VARIABLE void *spool_malloc(spool_t *pool, size_t size);
DLL_VARIABLE void *spool_try_realloc(spool_t *pool, void* p, size_t size);
DLL_VARIABLE void *spool_realloc(spool_t *pool, void* p, size_t size);
DLL_VARIABLE void spool_free(spool_t *pool, void *p);

typedef sstring_t sbuf_t;
DLL_VARIABLE int  spool_prepare_alloc(spool_t *pool, sbuf_t *buf);
DLL_VARIABLE void spool_rollback_alloc(spool_t *pool);
DLL_VARIABLE void *spool_commit_alloc(spool_t *pool, size_t length);
DLL_VARIABLE void spool_stat(spool_t *pool);

#ifdef DEBUG
typedef void (*cookie_cb_t)(void*);
extern cookie_cb_t cookie_cb;
#endif

#ifdef __cplusplus
};
#endif

#endif /* _SHTTP_POOL_H_INCLUDED_ */
