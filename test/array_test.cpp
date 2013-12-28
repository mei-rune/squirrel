
#include "platform.h"
#include "array.h"
#include "test.h"


typedef struct intArray_s
{
	ARRAY_DECLARE(int);
} intArray_t;


void int_init(int* begin, int* end)
{
  memset(begin, 0, (char*)end - (char*)begin);
}

void int_free(int val)
{  
}
  

TEST(ARRAY, func)
{
  intArray_t ar;

  ARRAY_INIT(int, &ar, 2);

  ARRAY_PUT(int, &ar, 0, 1);
  ARRAY_PUT(int, &ar, 1, 2);

  ASSERT_EQ(2, ar.capacity);
  ASSERT_EQ(1, ARRAY_GET(int, &ar,0,-1));
  ASSERT_EQ(2, ARRAY_GET(int, &ar,1,-1));


  ARRAY_PUT(int, &ar, 4, 5);
  ARRAY_PUT(int, &ar, 3, 4);

  ASSERT_EQ(8, ar.capacity);
  ASSERT_EQ(1, ARRAY_GET(int, &ar,0,-1));
  ASSERT_EQ(2, ARRAY_GET(int, &ar,1,-1));


  ASSERT_EQ(5, ARRAY_GET(int, &ar,4,-1));
  ASSERT_EQ(4, ARRAY_GET(int, &ar,3,-1));

  ASSERT_EQ(0, ARRAY_GET(int, &ar,2,-1));
  ASSERT_EQ(-1, ARRAY_GET(int, &ar,5,-1));

  ARRAY_DEL(int, &ar, 4, MOVE_FAST);
  ASSERT_EQ(-1, ARRAY_GET(int, &ar,4,-1));
}
