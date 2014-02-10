
#ifndef _squirrel_utils_h_
#define _squirrel_utils_h_ 1

#include "squirrel_config.h"

#ifndef _WIN32
#ifndef PORTABLE_SLIST_ENTRY
#define PORTABLE_SLIST_ENTRY(type) SLIST_ENTRY(type)
#endif
#endif

#ifdef _WIN32
#define snprintf    _snprintf
#define vsnprintf   _vsnprintf
#define vscprintf   _vscprintf
//#define strncmp     _strncmp
#define strncasecmp _strnicmp
#define strcasecmp  _stricmp
#define strtoll     _strtoi64
#define strtoull    _strtoui64
#define filelength  _filelength
#define fileno      _fileno
#ifndef _CRTDBG_MAP_ALLOC
#define getcwd      _getcwd
#endif
#define stat        _stat
#define access      _access

#define socket_t SOCKET
#define iovec WSABUF
#else
#define vscprintf(fmt, argList) vsnprintf(0, 0, fmt, argList)

#define socket_t int;
#define closesocket  close
#endif

#ifndef __cplusplus
#define inline __inline
#endif

#if __GNUC__
typedef long long int int64;
typedef unsigned long long int uint64;
#else
typedef __int64 int64;
typedef unsigned __int64 uint64;
#endif

#ifndef __RPCNDR_H__
typedef int boolean;
#endif

#ifndef false
#define false ((boolean)0)
#endif
#ifndef true
#define true ((boolean)1)
#endif

#ifndef nil
#ifdef NULL
#define nil NULL
#else
#define nil 0
#endif
#endif

#define sl_max(x,y) (((x) > (y))?(x):(y))
#define sl_min(x,y) (((x) < (y))?(x):(y))

#ifndef EMPTY
#define EMPTY
#endif

#define sl_malloc     malloc
#define sl_free(x)    if(nil != x) free(x)
#define sl_realloc    realloc
#define sl_calloc     calloc
#define sl_strdup     strdup

#ifndef SHTTP_CPU_CACHE_LINE
#define SHTTP_CPU_CACHE_LINE  32
#endif
#define shttp_align(d, a)         (((d) + (a - 1)) & ~(a - 1))
#define SHTTP_MEM_ALIGNMENT       sizeof(intptr_t)
#define shttp_mem_align(b)        shttp_align(b, SHTTP_MEM_ALIGNMENT)
#define shttp_align_ptr(p, a)     (char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

/* On Windows, variables that may be in a DLL must be marked specially.  */
#ifdef _MSC_VER
#ifdef _USRDLL
#if  SL_EXPORTS
# define DLL_VARIABLE __declspec (dllexport)
# define DLL_VARIABLE_C extern "C" __declspec (dllexport)
#else
# define DLL_VARIABLE __declspec (dllimport)
# define DLL_VARIABLE_C extern "C" __declspec (dllimport)
#endif
#else
# define DLL_VARIABLE extern
# define DLL_VARIABLE_C extern
#endif
#else
# define DLL_VARIABLE extern
# define DLL_VARIABLE_C extern
#endif



/**
 * @name Manipulation macros for struct timeval.
 *
 * We define replacements
 * for timeradd, timersub, timerclear, timercmp, and timerisset.
 *
 * @{
 */
#ifdef _SHTTP_HAVE_TIMERADD
#define shttp_timeradd(tvp, uvp, vvp) timeradd((tvp), (uvp), (vvp))
#define shttp_timersub(tvp, uvp, vvp) timersub((tvp), (uvp), (vvp))
#else
#define shttp_timeradd(tvp, uvp, vvp)                       \
  do {                                                      \
    (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;          \
    (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;       \
    if ((vvp)->tv_usec >= 1000000) {                        \
      (vvp)->tv_sec++;                                      \
      (vvp)->tv_usec -= 1000000;                            \
    }                                                       \
  } while (0)
#define shttp_timersub(tvp, uvp, vvp)                       \
  do {                                                      \
    (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
    (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
    if ((vvp)->tv_usec < 0) {                               \
      (vvp)->tv_sec--;                                      \
      (vvp)->tv_usec += 1000000;                            \
    }                                                       \
  } while (0)
#endif /* !_EVENT_HAVE_HAVE_TIMERADD */

#ifdef _SHTTP_HAVE_TIMERCLEAR
#define shttp_timerclear(tvp) timerclear(tvp)
#else
#define shttp_timerclear(tvp) (tvp)->tv_sec = (tvp)->tv_usec = 0
#endif
/**@}*/

/** Return true iff the tvp is related to uvp according to the relational
 * operator cmp.  Recognized values for cmp are ==, <=, <, >=, and >. */
#define shttp_timercmp(tvp, uvp, cmp)                       \
  (((tvp)->tv_sec == (uvp)->tv_sec) ?                       \
   ((tvp)->tv_usec cmp (uvp)->tv_usec) :                    \
   ((tvp)->tv_sec cmp (uvp)->tv_sec))

#ifdef _SHTTP_HAVE_TIMERISSET
#define shttp_timerisset(tvp) timerisset(tvp)
#else
#define shttp_timerisset(tvp) ((tvp)->tv_sec || (tvp)->tv_usec)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
DLL_VARIABLE int shttp_gettimeofday(struct timeval *tv, struct timezone *tz);
#else
#define shttp_gettimeofday  gettimeofday
#endif


#define LOG_HTTP(at, len, fmt, ...) _shttp_log_data(at, len, fmt "\n", __VA_ARGS__);
#define LOG(level, ...) _shttp_log_printf(level, __VA_ARGS__)
#define TRACE(...) _shttp_log_printf(SHTTP_LOG_DEBUG, __VA_ARGS__)
#define WARN(...)  _shttp_log_printf(SHTTP_LOG_WARN, __VA_ARGS__)
#define ERR(...) _shttp_log_printf(SHTTP_LOG_ERR, __VA_ARGS__)
#define CRIT(...)  _shttp_log_printf(SHTTP_LOG_CRIT, __VA_ARGS__)

// Log Functions
void _shttp_log_data(const char* data, size_t len, const char* fmt, ...);
extern void _shttp_log_printf(int severity, const char *fmt, ...);


typedef void (*shttp_assert_cb_t)(void* ctx, const char* msg, const char* file, int line);
extern void              *shttp_assert_ctx;
extern shttp_assert_cb_t  shttp_assert_cb;

#define shttp_assert_for_cb(x) do{ if(nil == shttp_assert_cb) { assert(x); } else { if(!(x)) { shttp_assert_cb(shttp_assert_ctx, #x, __FILE__, __LINE__); } } } while(0)
#ifdef DEBUG
#define shttp_assert(x) shttp_assert_for_cb(x)
#else
#define shttp_assert(x)
#endif

extern int shttp_win32_version;
extern int shttp_ncpu;
extern int shttp_cacheline_size;
extern int shttp_pagesize;
extern int shttp_pagesize_shift;
DLL_VARIABLE void os_init();


#ifdef __cplusplus
};
#endif

#endif //_INTERNAL_H_
