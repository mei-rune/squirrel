
#ifndef _squirrel_config_h_
#define _squirrel_config_h_ 1

#define _CRTDBG_MAP_ALLOC

#ifdef _WIN32
// 定义此宏表示将在 win2003 上运行
#ifndef _WIN32_WINNT
#define _WIN32_WINNT  0x0501
#endif
#if _MSC_VER >= 1600
#include <SDKDDKVer.h>
#endif
#include <winsock2.h>
#include <windows.h>
#include <Mswsock.h>
#include <Ws2tcpip.h>


#ifndef __MINGW32__
#include <SDKDDKVer.h>
#if (NTDDI_VERSION >= NTDDI_VISTA)
# define HAS_INET_NTOP 1
#endif
#endif

#else
# define HAS_INET_NTOP 1
#endif

//#define SHTTP_DEBUG 1

#endif //_squirrel_config_h_