

#ifndef _squirrel_sstring_h_
#define _squirrel_sstring_h_ 1

#include "squirrel_config.h"

// Include files
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#include "squirrel_util.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sstring_s {
  size_t          len;
  char*           str;
} sstring_t;

typedef struct cstring_s {
  size_t          len;
  const char*     str;
} cstring_t;

#define  SSTRING_DEFAULT  { 0, NULL }
#define  sstring_init(s, p, l) (s).str = (p); (s).len = (l);
#define  sstring_data(s) (s).str
#define  sstring_length(s) (s).len
#define  sstring_clear(s) (s).len = 0
#define  sstring_assign(s, v) (s).len = (v).len; (s).str = (v).str
#define  sstring_truncate(s, l) (s).len = (l)
#define  sstring_swap(lhs, rhs) do {       \
  size_t          len;                     \
  const char*     str;                     \
  len = lhs.len;                           \
  str = lhs.str;                           \
  lhs.len = rhs.len;                       \
  lhs.str = rhs.str;                       \
  rhs.len = len;                           \
  rhs.str = str;                           \
} while(0)



typedef struct sbuffer_s {
  size_t          len;
  char*           str;
  size_t          capacity;
} sbuffer_t;

#define  SSTRING_BUFFER_DEFAULT  { 0, NULL, 0 }

#define  sbuffer_init(s, p, l, c) (s).str = (p); (s).len = (l); (s).capacity = (c)
#define  sbuffer_zero(s) sbuffer_init(s, nil, 0, 0)
#define  sbuffer_unused_size(s) ((s).capacity - (s).len)
#define  sbuffer_unused_addr(s) ((s).str + (s).len)
#define  sbuffer_length_add(s, l) (s).len += (l)
#define  sbuffer_length_set(s, l) (s).len = (l)
#define  sbuffer_length(s)        (s).len

DLL_VARIABLE void sbuffer_destroy(sbuffer_t* pcs);

DLL_VARIABLE void sbuffer_release(sbuffer_t* lhs, sstring_t* rhs);

DLL_VARIABLE void sbuffer_swap_to(sbuffer_t* lhs, sstring_t* rhs);

DLL_VARIABLE void sbuffer_swap(sbuffer_t* lhs, sbuffer_t* rhs);

DLL_VARIABLE sbuffer_t* sbuffer_assign(sbuffer_t* pcs, const char* s);

DLL_VARIABLE sbuffer_t* sbuffer_assignLen(sbuffer_t* pcs, const char* s, size_t cch);

DLL_VARIABLE sbuffer_t* sbuffer_assignN(sbuffer_t* pcs, char ch, size_t n);

DLL_VARIABLE sbuffer_t* sbuffer_vsprintf(sbuffer_t* pcs, const char*fmt, va_list argList);

DLL_VARIABLE sbuffer_t* sbuffer_sprintf(sbuffer_t* pcs, const char*fmt, ...);

DLL_VARIABLE sbuffer_t* sbuffer_append(sbuffer_t*   pcs
                                       , char const*         s);

DLL_VARIABLE sbuffer_t* sbuffer_appendLen(sbuffer_t*   pcs
    , char const*         s
    , size_t              len);

DLL_VARIABLE sbuffer_t* sbuffer_appendN(sbuffer_t*   pcs
                                        , char                ch
                                        , size_t              n);

DLL_VARIABLE sbuffer_t* sbuffer_append_vsprintf(sbuffer_t* pcs
    , const char*         fmt
    , va_list             argList);

DLL_VARIABLE sbuffer_t* sbuffer_append_sprintf(sbuffer_t* pcs, const char*fmt, ...);

DLL_VARIABLE sbuffer_t* sbuffer_append_from_file(sbuffer_t* pcs, const char* name);



static inline int cstring_hash(const cstring_t *k) {
  int hash = 5381;
  size_t i;
  const char *key = (const char *) k->str;

  for (i = 0; i < k->len; i++)
    hash = ((hash << 5) + hash) + key[i]; /* hash * 33 + char */

  return hash;
}

static inline int cstring_cmp(const cstring_t *k1, const cstring_t *k2) {
  if(k1->len == k2->len)
    return memcmp(k1->str, k2->str, k1->len);
  else if(k1->len > k2->len)
    return 1;
  else
    return -1;
}

static inline int sstring_hash(const sstring_t *k) {
  int hash = 5381;
  size_t i;
  const char *key = (const char *) k->str;

  for (i = 0; i < k->len; i++)
    hash = ((hash << 5) + hash) + key[i]; /* hash * 33 + char */

  return hash;
}

static inline int sstring_cmp(const sstring_t *k1, const sstring_t *k2) {
  if(k1->len == k2->len)
    return memcmp(k1->str, k2->str, k1->len);
  else if(k1->len > k2->len)
    return 1;
  else
    return -1;
}



static inline int strhash(const char *k) {
  int hash = 5381;
  size_t i, len;
  const char *key;

  key = (const char *) k;
  len = strlen(k);

  for (i = 0; i < len; i++)
    hash = ((hash << 5) + hash) + key[i]; /* hash * 33 + char */

  return hash;
}

typedef struct stoken_ctx_s {
  const char *start;
  size_t      len;
  const char *delimit_str;
  size_t      delimit_len;
} stoken_ctx_t;

static inline void stoken_init(stoken_ctx_t *ctx, const char* txt, size_t len, const char *delimit_str, size_t delimit_len) {
  ctx->start = txt;
  ctx->len = (-1 == len)?strlen(txt):len;
  ctx->delimit_str = delimit_str;
  ctx->delimit_len =  (-1 == delimit_len)?strlen(delimit_str):delimit_len;
}

DLL_VARIABLE int stoken_next(stoken_ctx_t *ctx, cstring_t *buf);

#ifdef __cplusplus
};
#endif


#endif // _string_buffer_h_

