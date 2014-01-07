
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/timeb.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#endif
#include "squirrel_slab.h"

#ifdef _WIN32
/* No gettimeofday; this muse be windows. */
int gettimeofday(struct timeval *tv, struct timezone *tz) {
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

uint64_t usTime() {
  struct timeval tv;
  uint64_t usec;

  gettimeofday(&tv, NULL);

  usec = ((uint64_t)tv.tv_sec)*1000000LL;
  usec += tv.tv_usec;

  return usec;
}

int main(int argc, char **argv) {
  shttp_slab_pool_t *sp;
  size_t  pool_size;
  char  *space;

  pool_size = 4096000;  //4M
  space = (char *)malloc(pool_size);
  sp = (shttp_slab_pool_t*) space;

  sp->addr = space;
  sp->min_shift = 3;
  sp->end = space + pool_size;

  shttp_slab_init(sp);

  char *p;
  uint64_t us_begin;
  uint64_t us_end;
  size_t size[] = { 30, 120, 256, 500, 1000, 2000, 3000, 5000};

  printf("size\tncx\tmalloc\n");

  int i, j;
  for (j = 0; j < sizeof(size)/sizeof(size_t); j++) {
    size_t s = size[j];
    printf("%d\t", s);

    //test for shttp_pool
    us_begin  = usTime();
    for(i = 0; i < 1000000; i++) {
      p = shttp_slab_alloc(sp, s);
      shttp_slab_free(sp, p);
    }
    us_end  = usTime();

    printf("%llu \t", (us_end - us_begin)/1000);

    // test for malloc
    us_begin  = usTime();
    for(i = 0; i < 1000000; i++) {
      p = (char*)malloc(s);
      free(p);
    }
    us_end  = usTime();

    printf("%llu\n", (us_end - us_begin)/1000);
  }


  free(space);

  return 0;
}
