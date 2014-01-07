
#include "squirrel_config.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "squirrel_string.h"

#define CSTRING_ALLOC_GRANULARITY   (8)
#define CSTRING_OFFSET_SIZE         (16)


#define cstring_assert(expr)        assert(expr)

#ifdef __cplusplus
extern "C"
{
#endif

static size_t string_strlen_safe_(char const* s) {
  return (NULL == s) ? 0 : strlen(s);
}

static void sbuffer_ensure_len(sbuffer_t* pcs, size_t len) {
  char*       newPtr = NULL;
  size_t  capacity = (0 == len) ? 1u : len;


  cstring_assert(NULL != pcs);

  if (0 != pcs->capacity &&
      pcs->capacity > len)
    return ;


  capacity  = (capacity + (CSTRING_ALLOC_GRANULARITY - 1)) & ~(CSTRING_ALLOC_GRANULARITY - 1);
  if (capacity < pcs->capacity * 2)
    capacity = pcs->capacity * 2;

  if(0 == pcs->str)
    newPtr = (char*)sl_malloc(capacity + 1);
  else
    newPtr = (char*)sl_realloc(pcs->str, capacity + 1);

  pcs->str = newPtr;
  pcs->str[pcs->len]  =   '\0';
  pcs->capacity = capacity;

  return ;
}

DLL_VARIABLE void sbuffer_destroy(sbuffer_t* pcs) {
  if(NULL == pcs)
    return;
  sl_free(pcs->str);
  pcs->str = 0;
  pcs->len = 0;
  pcs->capacity = 0;
}

DLL_VARIABLE sbuffer_t* sbuffer_assign(sbuffer_t*   pcs
                                       ,   char const*         s) {
  const size_t len = string_strlen_safe_(s);
  sbuffer_assignLen(pcs, s, len);
  return pcs;
}

DLL_VARIABLE sbuffer_t* sbuffer_assignLen(sbuffer_t*       pcs
    ,       char const*             s
    ,       size_t                  len) {
  sbuffer_ensure_len(pcs, len);
  strncpy(pcs->str, s, len);
  pcs->len = len;
  pcs->str[pcs->len] = '\0';
  return pcs;
}

DLL_VARIABLE sbuffer_t* sbuffer_assignN(sbuffer_t*       pcs
                                        , char   ch
                                        , size_t n) {
  sbuffer_ensure_len(pcs, n);
  memset(pcs->str, ch, n);
  pcs->len = n;
  pcs->str[pcs->len] = '\0';
  return pcs;
}


DLL_VARIABLE void string_vsprintf_(sbuffer_t* pcs, size_t begin, const char* fmt, va_list argList) {
  int len = 0;

  sbuffer_ensure_len(pcs, pcs->len + 200);
  len = vsnprintf(pcs->str + begin, pcs->capacity - begin, fmt, argList);
  if(len > 0) {
    pcs->len = begin + len;
    return;
  }

  len = vscprintf(fmt, argList);
  if(len <= 0)
    return ;

  sbuffer_ensure_len(pcs, begin + len + 10);
  len = vsnprintf(pcs->str + begin, pcs->capacity - begin, fmt, argList);
  if(len > 0) {
    pcs->len = begin + len;
    return ;
  }
  return;
}

DLL_VARIABLE sbuffer_t* sbuffer_vsprintf(sbuffer_t* pcs, const char*fmt, va_list argList) {
  string_vsprintf_(pcs, 0, fmt, argList);
  return pcs;
}

DLL_VARIABLE sbuffer_t* sbuffer_sprintf(sbuffer_t* pcs, const char*fmt, ...) {
  va_list argList;
  va_start(argList, fmt);
  string_vsprintf_(pcs, 0, fmt, argList);
  va_end( argList );
  return pcs;
}


DLL_VARIABLE sbuffer_t* sbuffer_append_vsprintf(sbuffer_t* pcs, const char*fmt, va_list argList) {
  string_vsprintf_(pcs, pcs->len, fmt, argList);
  return pcs;
}

DLL_VARIABLE sbuffer_t* sbuffer_append_sprintf(sbuffer_t* pcs, const char*fmt, ...) {
  va_list argList;
  va_start(argList, fmt);
  string_vsprintf_(pcs, pcs->len, fmt, argList);
  va_end( argList );
  return pcs;
}

DLL_VARIABLE sbuffer_t* sbuffer_append(sbuffer_t*   pcs
                                       ,   char const*         s) {
  const size_t len = string_strlen_safe_(s);
  sbuffer_appendLen(pcs, s, len);
  return pcs;
}

DLL_VARIABLE sbuffer_t* sbuffer_appendLen(sbuffer_t*   pcs
    ,       char const*         s
    ,       size_t              len) {
  size_t newLen = 0;

  cstring_assert(NULL != pcs);
  cstring_assert(NULL != s || 0 == len);

  newLen = pcs->len + len;
  sbuffer_ensure_len(pcs, newLen);

  strncpy(pcs->str + pcs->len, s, len);
  pcs->len = newLen;
  pcs->str[pcs->len] = '\0';
  return pcs;
}

DLL_VARIABLE sbuffer_t* sbuffer_appendN(sbuffer_t*   pcs
                                        ,       char                ch
                                        ,       size_t              n) {
  size_t newLen = pcs->len + n;
  sbuffer_ensure_len(pcs, newLen);

  memset(pcs->str + pcs->len, ch, n);
  pcs->len = newLen;
  pcs->str[pcs->len] = '\0';
  return pcs;
}


DLL_VARIABLE void sbuffer_swap(sbuffer_t* lhs, sbuffer_t* rhs) {
  sbuffer_t tmp;
  memcpy(&tmp, lhs, sizeof(sbuffer_t));
  memcpy(lhs, rhs, sizeof(sbuffer_t));
  memcpy(rhs, &tmp, sizeof(sbuffer_t));
}

DLL_VARIABLE void sbuffer_swap_to(sbuffer_t* lhs, sstring_t* rhs) {
  sstring_t tmp;
  tmp.str = rhs->str;
  tmp.len = rhs->len;

  rhs->str = lhs->str;
  rhs->len = lhs->len;

  lhs->str = tmp.str;
  lhs->len = tmp.len;
  lhs->capacity =  tmp.len;
}

DLL_VARIABLE void sbuffer_release(sbuffer_t* lhs, sstring_t* rhs) {
  if(0 != rhs->str)
    sl_free(rhs->str);

  rhs->str = lhs->str;
  rhs->len = lhs->len;

  lhs->str = 0;
  lhs->len = 0;
  lhs->capacity = 0;
}

DLL_VARIABLE sbuffer_t* sbuffer_append_from_file(sbuffer_t* pcs, const char* name) {

  int num;
  size_t newLen;
  FILE* stream = fopen(name, "r");
  if(0 == stream)
    return NULL;

  while(!feof(stream)) {
    newLen = pcs->len + 1024;
    sbuffer_ensure_len(pcs, newLen);

    num = fread(pcs->str + pcs->len, sizeof(char), 1024, stream);
    if(ferror(stream)) {
      num = -1;
      break;
    }
    pcs->len += num;
  }

  fclose(stream);
  return (-1 == num)?NULL:pcs;
}

DLL_VARIABLE sbuffer_t* sbuffer_insertLen(sbuffer_t*       pcs
    ,   int                     index
    ,   char const*             s
    ,   size_t                  len ) {
  size_t  realIndex;

  if (0 >= len)
    return pcs;

  cstring_assert(NULL != pcs);

  if (index < 0) {
    if (-index > (int)pcs->len) {
      errno = EINVAL;
      return 0;
    }
    realIndex = pcs->len - (size_t)(-index);
  } else {
    realIndex = (size_t)index;
    if (realIndex > pcs->len) {
      errno = EINVAL;
      return 0;
    }
  }


  if (len > pcs->capacity - pcs->len)
    sbuffer_ensure_len(pcs, pcs->len + len);

  /* copy over the rhs of the std::string */
  memmove(pcs->str + realIndex + len, pcs->str + realIndex, (pcs->len - realIndex) * sizeof(char));
  pcs->len += len;
  memcpy(pcs->str + realIndex, s, sizeof(char) * len);
  pcs->str[pcs->len] = '\0';
  return pcs;
}

DLL_VARIABLE sbuffer_t* sbuffer_insert(sbuffer_t*       pcs
                                       ,   int                     index
                                       ,   char const*             s ) {
  const size_t cch = string_strlen_safe_(s);
  return sbuffer_insertLen(pcs, index, s, cch);
}

DLL_VARIABLE int sbuffer_replaceLen(sbuffer_t*       pcs
                                    ,   int                     index
                                    ,   size_t                  len
                                    ,   char const*             s
                                    ,   size_t                  cch ) {
  size_t  realIndex;

  cstring_assert(NULL != pcs);

  if (index < 0) {
    if (-index > (int)pcs->len) {
      errno = EINVAL;
      return -1;
    }
    realIndex = pcs->len - (size_t)(-index);
  } else {
    realIndex = (size_t)index;
    if (realIndex + len > pcs->len) {
      errno = EINVAL;
      return -1;
    }
  }


  if (cch > len)
    sbuffer_ensure_len(pcs, pcs->len + (cch - len));

  if (len != cch) {
    /* copy over the rhs of the std::string */
    const size_t n = pcs->len - (realIndex + len);

    memmove(pcs->str + realIndex + cch, pcs->str + realIndex + len, n * sizeof(char));

    if (cch > len)
      pcs->len += (cch - len);
    else
      pcs->len -= (len - cch);

    pcs->str[pcs->len] = '\0';
  }

  /* we can simply replace directly */
  memcpy(pcs->str + realIndex, s, sizeof(char) * cch);

  return 0;
}

DLL_VARIABLE int sbuffer_replace(sbuffer_t*       pcs
                                 ,   int                     index
                                 ,   size_t                  len
                                 ,   char const*             s ) {
  const size_t cch = string_strlen_safe_(s);
  return sbuffer_replaceLen(pcs, index, len, s, cch);
}

DLL_VARIABLE int sbuffer_replaceAll(sbuffer_t*       pcs
                                    ,   char const*             f
                                    ,   char const*             t
                                    ,   size_t*                 numReplaced /* = NULL */ ) {
  size_t  numReplaced_;

  cstring_assert(NULL != pcs);

  if (NULL == numReplaced)
    numReplaced = &numReplaced_;

  *numReplaced = 0;

  if (NULL == f ||
      '\0' == f[0])
    return 0;

  if (0 == pcs->len)
    return 0;

  {
    /* Search for first, then replace, then repeat from search pos */
    const size_t  flen  =   strlen(f);
    const size_t  tlen  =   string_strlen_safe_(t);
    size_t        pos   =   0;
    char*         p;

    for (; NULL != (p = strstr(pcs->str + pos, f)); pos += tlen) {
      int rc;

      pos = (size_t)(p - pcs->str);

      rc = sbuffer_replaceLen(pcs, (int)pos, flen, t, tlen);

      if (0 != rc)
        return rc;
    }
  }
  return 0;
}

#ifdef __cplusplus
};
#endif
