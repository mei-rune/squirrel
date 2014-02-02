#include "squirrel_config.h"
#include <stdlib.h>
#include "squirrel_ring.h"
#include "squirrel_test.h"

RING_BUFFER_DEFINE(int_buffer, long)

void check_int_buffer(out_fn_t out_fn, int_buffer_t* cb, int c, boolean isNoCommit) {
  int    i =0;
  long * elem;

  if (c < 10) {
    if (isNoCommit) {
      ASSERT_EQ(c, int_buffer_size(cb));
    } else {
      ASSERT_EQ(c+1, int_buffer_size(cb));

      ring_buffer_foreach(i, elem, cb) {
        ASSERT_EQ(i, *elem);
        ASSERT_EQ(int_buffer_get(cb, i), elem);
      }


      if (0 != int_buffer_size(cb)) {
        if (isNoCommit) {
          ASSERT_EQ((c-1), *int_buffer_last(cb));
        } else {
          ASSERT_EQ(c, *int_buffer_last(cb));
        }

        ASSERT_EQ(0, *int_buffer_first(cb));
      }
    }
  } else {
    if (isNoCommit) {
      ASSERT_EQ(9, int_buffer_size(cb));
    } else {
      ASSERT_EQ(10, int_buffer_size(cb));
    }

    ring_buffer_foreach(i, elem, cb) {
      ASSERT_EQ(c-9+i, *elem);
      ASSERT_EQ(int_buffer_get(cb, i), elem);
    }

    if (isNoCommit) {
      ASSERT_EQ(c-1, *int_buffer_last(cb));
    } else {
      ASSERT_EQ(c, *int_buffer_last(cb));
    }

    ASSERT_EQ(c-9, *int_buffer_first(cb));
  }
}

TEST(int_buffer, simple) {
  int             i;
  int_buffer_t   rb;
  long           *t;

  int_buffer_init(&rb, (long*)malloc(10 * sizeof(long)), 10);
  for(i = 0; i < 100; i++) {
    int_buffer_prepare(&rb);
    check_int_buffer(out_fn, &rb, i, true);

    t = int_buffer_prepare(&rb);
    *t = i;
    int_buffer_push(&rb);
    check_int_buffer(out_fn, &rb, i, false);
  }
}
