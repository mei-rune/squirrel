#include "squirrel_config.h"
#include <stdint.h>
#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

int shttp_win32_version = 0;
int shttp_pagesize = 0;
int shttp_ncpu = 0;
int shttp_cacheline_size = SHTTP_CPU_CACHE_LINE;
int shttp_pagesize_shift = 0;

void
os_init() {
  DWORD        bytes;
  //ev_err_t     err;
  uint32_t    n;
  SYSTEM_INFO  si;
  u_int               osviex;
  OSVERSIONINFOEX     osvi;


  /* get Windows version */

  memset(&osvi, 0, sizeof(OSVERSIONINFOEX));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

  osviex = GetVersionEx((OSVERSIONINFO *) &osvi);

  if (osviex == 0) {
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((OSVERSIONINFO *) &osvi) == 0) {
      ERR("GetVersionEx() failed, errno=%d", GetLastError());
      return ;
    }
  }

  /*
   *  Windows 3.1 Win32s   0xxxxx
   *
   *  Windows 95           140000
   *  Windows 98           141000
   *  Windows ME           149000
   *  Windows NT 3.51      235100
   *  Windows NT 4.0       240000
   *  Windows NT 4.0 SP5   240050
   *  Windows 2000         250000
   *  Windows XP           250100
   *  Windows 2003         250200
   *  Windows Vista/2008   260000
   *
   *  Windows CE x.x       3xxxxx
   */

  shttp_win32_version = osvi.dwPlatformId * 100000
                        + osvi.dwMajorVersion * 10000
                        + osvi.dwMinorVersion * 100;

  if (osviex) {
    shttp_win32_version += osvi.wServicePackMajor * 10
                           + osvi.wServicePackMinor;
  }

  GetSystemInfo(&si);
  shttp_pagesize = si.dwPageSize;
  shttp_ncpu = si.dwNumberOfProcessors;
  shttp_cacheline_size = SHTTP_CPU_CACHE_LINE;

  // compute shttp_pagesize_shift
  for (n = shttp_pagesize; n >>= 1; shttp_pagesize_shift++) {
    /* void */
  }
}


#ifdef __cplusplus
};
#endif