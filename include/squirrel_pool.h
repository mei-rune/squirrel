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


#ifdef SHTTP_DEBUG
typedef void*                            spool_t;
#define spool_alloc_size(size)           shttp_align(size, shttp_cacheline_size)
#define spool_excepted(s)                (spool_alloc_size(s))


#define spool_init(pool, p, capacity)
#define spool_malloc(pool, size)               malloc(size)
#define spool_try_realloc(pool, p, size)       nil
#define spool_realloc(pool, p, size)           realloc(p, size)
#define spool_free(pool, p)                    free(p)

typedef sstring_t sbuf_t;
static inline int spool_prepare_alloc(spool_t *pool, sbuf_t *buf) {
  (buf)->str = (char*)malloc(20*1024);
  (buf)->len = (20*1024);
  return 0;
}
#define spool_rollback_alloc(pool, buf)    free((buf)->str)
#define spool_commit_alloc(pool, length)
#define spool_stat(pool)
#else

typedef struct spool_block_s {
  uint16_t                    magic;
  uint16_t                    used:1;
  uint16_t                    reserved:15;
#ifdef DEBUG
  uint32_t                    reserved2;
  uint32_t                    used_size;
#endif
  uint32_t                    capacity;
#ifdef DEBUG
  const char                  *file;
  int                         line;
#endif
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
DLL_VARIABLE void *spool_malloc_dbg(spool_t *pool, size_t size, const char* file, int line);
DLL_VARIABLE void *spool_try_realloc_dbg(spool_t *pool, void* p, size_t size, const char* file, int line);
DLL_VARIABLE void *spool_realloc_dbg(spool_t *pool, void* p, size_t size, const char* file, int line);
DLL_VARIABLE void spool_free_dbg(spool_t *pool, void *p, const char* file, int line);

typedef sstring_t sbuf_t;
DLL_VARIABLE int  spool_prepare_alloc_dbg(spool_t *pool, sbuf_t *buf, const char* file, int line);
DLL_VARIABLE void spool_rollback_alloc_dbg(spool_t *pool, const char* file, int line);
DLL_VARIABLE void *spool_commit_alloc_dbg(spool_t *pool, size_t length, const char* file, int line);
DLL_VARIABLE void spool_stat(spool_t *pool);

#define spool_malloc(pool, size)               spool_malloc_dbg(pool, size, __FILE__, __LINE__)
#define spool_try_realloc(pool, p, size)       spool_try_realloc_dbg(pool, p, size, __FILE__, __LINE__)
#define spool_realloc(pool, p, size)           spool_realloc_dbg(pool, p, size, __FILE__, __LINE__)
#define spool_free(pool, p)                    spool_free_dbg(pool, p, __FILE__, __LINE__)
#define spool_prepare_alloc(pool, buf)         spool_prepare_alloc_dbg(pool, buf, __FILE__, __LINE__)
#define spool_rollback_alloc(pool, buf)        spool_rollback_alloc_dbg(pool, __FILE__, __LINE__)
#define spool_commit_alloc(pool, length)       spool_commit_alloc_dbg(pool, length, __FILE__, __LINE__)

#ifdef DEBUG
typedef void (*cookie_cb_t)(void* ctx, void* block);
extern cookie_cb_t cookie_cb;
extern void* cookie_ctx;
#endif

#endif


#ifdef __cplusplus
};
#endif

#endif /* _SHTTP_POOL_H_INCLUDED_ */
