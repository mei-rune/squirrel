
#ifndef _squirrel_utils_h_
#define _squirrel_utils_h_ 1

#include "squirrel_config.h"

#ifndef _WIN32
 #ifndef PORTABLE_SLIST_ENTRY
  #define PORTABLE_SLIST_ENTRY(type) SLIST_ENTRY(type)
 #endif
#endif

#ifdef _WIN32
 #define snprintf _snprintf
 #define vsnprintf _vsnprintf
 #define vscprintf _vscprintf
 #define strncasecmp _strnicmp
 #define strcasecmp _stricmp
 #define strtoll _strtoi64
 #define strtoull _strtoui64
 #define filelength _filelength
 #define fileno _fileno
 #define getcwd _getcwd
 #define stat _stat
 #define access _access

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
 #define nil 0
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
#define shttp_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define SHTTP_MEM_ALIGNMENT       sizeof(intptr_t)
#define shttp_mem_align(b) shttp_align(b, SHTTP_MEM_ALIGNMENT)


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

#endif //_INTERNAL_H_
