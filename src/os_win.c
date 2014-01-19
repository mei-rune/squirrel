#include "squirrel_config.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

int shttp_win32_version = 0;
int shttp_pagesize = 0;
int shttp_ncpu = 0;
int shttp_cacheline_size = SHTTP_CPU_CACHE_LINE;
int shttp_pagesize_shift = 0;

int cache_line_size();

void os_init() {
  //DWORD        bytes;
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
  shttp_cacheline_size = cache_line_size();
  if(32 > shttp_cacheline_size) {
    shttp_cacheline_size = SHTTP_CPU_CACHE_LINE;
  }

  /*
   * pagesize = pow(2, pagesize_shift)，即xxx_shift都是指幂指数
   * 一块page被切割成大小相等的内存块(下面的注释暂称为obj)，
   * 不同的page，被切割的obj大小可能不等，但obj的大小都是2的N次方分配的.即 8 16 32 ...
   * */
  for (n = shttp_pagesize; n >>= 1; shttp_pagesize_shift++) {
    /* void */
  }

  _shttp_status_init();
}

#ifdef _WIN32
/* No gettimeofday; this muse be windows. */
int shttp_gettimeofday(struct timeval *tv, struct timezone *tz) {
  struct _timeb tb;

  if (tv == NULL)
    return -1;

  /* XXXX
   * _ftime is not the greatest interface here; GetSystemTimeAsFileTime
   * would give us better resolution, whereas something cobbled together
   * with GetTickCount could maybe give us monotonic behavior.
   *
   * Either way, I think this value might be skewed to ignore the
   * timezone, and just return local time.  That's not so good.
   */
  _ftime(&tb);
  tv->tv_sec = (long) tb.time;
  tv->tv_usec = ((int) tb.millitm) * 1000;
  return 0;
}
#endif

#ifdef __cplusplus
};
#endif