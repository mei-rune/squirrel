#include "squirrel_config.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <Windows.h>
#include <Dbghelp.h>
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

static USHORT (WINAPI *s_pfnCaptureStackBackTrace)(ULONG, ULONG, PVOID*, PULONG) = 0;  
  

void os_init() {
  //DWORD        bytes;
  uint32_t    n;
  SYSTEM_INFO  si;
  u_int               osviex;
  OSVERSIONINFOEX     osvi;

    if (0 == s_pfnCaptureStackBackTrace) {  
        const HMODULE hNtDll = ::GetModuleHandle("ntdll.dll");  
        reinterpret_cast<void*&>(s_pfnCaptureStackBackTrace)
           =  ::GetProcAddress(hNtDll, "RtlCaptureStackBackTrace");
        if(nil == s_pfnCaptureStackBackTrace) {
          printf("[WARN] init CaptureStackBackTrace ailed");
        }
    }

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



#if defined(__linux__) || defined(__APPLE__)
// Unix
static void printStack(FILE *fp) {
        void *buffer[100];
        int nptrs;
        char **strings;
        int i;

        fputs("\n# backtrace\n", fp);

        /* include a backtrace for good measure */
        nptrs = backtrace(buffer, SIZE);
        strings = backtrace_symbols(buffer, nptrs);
        for (i = 0; i < nptrs; i++) {
                fputs(strings[i], fp);
                fputc('\n', fp);
        }

        free(strings);
}

#elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
// Windows
static void printStack(FILE *fp)
{
     unsigned int   i;
     void         * stack[ 100 ];
     unsigned short frames;
     SYMBOL_INFO  * symbol;
     HANDLE         process;
     if(nil == s_pfnCaptureStackBackTrace) {
       return;
     }

     process = GetCurrentProcess();

     SymInitialize( process, NULL, TRUE );

     frames               = s_pfnCaptureStackBackTrace( 0, 100, stack, NULL );
     symbol               = (SYMBOL_INFO*)calloc( sizeof(SYMBOL_INFO) + _MAX_PATH + 1, 1);
     symbol->MaxNameLen   = _MAX_PATH;
     symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

     for( i = 0; i < frames; i++ ) {
         SymFromAddr( process, ( DWORD64 )( stack[ i ] ), 0, symbol );
         ERR( "%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address);
     }

     free( symbol );
}

#elif defined(_MSC_VER)

  static int
  get_backtrace (void **backtrace_, int size_)
  {
    STACKFRAME64 stack_frame = {0};
    CONTEXT context = {0};
    HANDLE thread = GetCurrentThread ();
    HANDLE process = GetCurrentProcess ();
    if (!thread)
      {
        throw bs_exception ("get_backtrace", "Can't get thread handler");
      }
    if (!process)
      {
        throw bs_exception ("get_backtrace", "Can't get process handler");
      }

    static bool sym_init = true;
    if (sym_init)
      {
        if (!SymInitialize (process, "d:\\blue-sky\\exe\\release\\plugins", TRUE))
          {
            throw bs_exception ("get_backtrace", "Can't initialize symbol handler");
          }
        sym_init = false;
      }

    context.ContextFlags = CONTEXT_CONTROL;
    __asm
      {
      label_:
        mov [context.Ebp], ebp;
        mov [context.Esp], esp;
        mov eax, [label_];
        mov [context.Eip], eax;
      };

    stack_frame.AddrPC.Offset = context.Eip;
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Offset = context.Ebp;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Offset = context.Esp;
    stack_frame.AddrStack.Mode = AddrModeFlat;

    BOOL res = FALSE;
    int i = 0;
    for (; i < size_; ++i)
      {
        res = StackWalk64 (
          IMAGE_FILE_MACHINE_I386,
          process,
          thread,
          &stack_frame,
          NULL,
          NULL,
          SymFunctionTableAccess64,
          SymGetModuleBase64,
          NULL);

        if (!res || stack_frame.AddrPC.Offset == 0)
          break;

        backtrace_[i] = (void *)stack_frame.AddrPC.Offset;
      }

    if (!res && !i)
      {
        throw bs_exception ("get_backtrace", "Can't obtain call-stack info");
      }

    return i;
  }

  static char **
  get_backtrace_names (void *const *backtrace_, int size_)
  {
    char symbol_ [sizeof (IMAGEHLP_SYMBOL64) + sizeof (TCHAR) * (MAX_PATH + 1)] = {0};
    IMAGEHLP_SYMBOL64 *symbol = (IMAGEHLP_SYMBOL64 *)symbol_;

    symbol->SizeOfStruct = sizeof (IMAGEHLP_SYMBOL64);
    symbol->MaxNameLength = MAX_PATH;

    HANDLE process = GetCurrentProcess ();
    char **names = (char **)malloc ((MAX_PATH + 1 + sizeof (char *)) * size_);
    memset (names, 0, (MAX_PATH + 1 + sizeof (char *)) * size_);

    for (int i = 0; i < size_; ++i)
      {
        names[i] = (char *)names + sizeof (char *) * size_ + (MAX_PATH + 1) * i;

        BOOL res = SymGetSymFromAddr64 (process, (DWORD64)backtrace_[i], 0, symbol);
        if (!res)
          {
            LPVOID lpMsgBuf;
            DWORD dw = GetLastError();

            FormatMessage(
              FORMAT_MESSAGE_ALLOCATE_BUFFER |
              FORMAT_MESSAGE_FROM_SYSTEM |
              FORMAT_MESSAGE_IGNORE_INSERTS,
              NULL,
              dw,
              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
              (LPTSTR) &lpMsgBuf,
              0, NULL );

            BSERROR << (char*)lpMsgBuf << bs_end;
            LocalFree (lpMsgBuf);

            break;
          }

        memcpy (names[i], symbol->Name, (std::min <size_t>) (MAX_PATH, strlen (symbol->Name)));
      }

    return names;
  }
#endif

#ifdef __cplusplus
};
#endif