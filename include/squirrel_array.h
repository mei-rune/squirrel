#ifndef _arraylist_h_
#define _arraylist_h_ 1


#define ARRAY_DECLARE(type)                                                              \
    size_t capacity;                                                                     \
    size_t length;                                                                       \
    type* array


#define ARRAY_INIT(type, arr, size)                                                      \
  (arr)->capacity = size;                                                                \
  (arr)->length = 0;                                                                     \
  (arr)->array = (type *)sl_malloc((size) * sizeof( type ));                             \
  memset((arr)->array,0,(size) * sizeof( type ))

#define ARRAY_DESTROY(type, arr)     sl_free((arr)->array)


#define ARRAY_LENGTH(type, arr)  (arr)->length


#define ARRAY_CLEAR(type, arr)  (arr)->length = 0



#define ARRAY_SET(type, arr, idx, value)  (arr)->array[idx] = (value); (arr)->length += 1

#define ARRAY_GET(type, arr, idx, default_value)                                        \
 (((idx) >= (arr)->length)?(type)(default_value):(arr)->array[idx])

#define ARRAY_PUT(type, arr, idx, data)                                                 \
do {                                                                                    \
  if((idx) >= (arr)->capacity)                                                          \
  {                                                                                     \
    size_t old_idx = (arr)->length;                                                     \
    (arr)->capacity = 2*(idx);                                                          \
    (arr)->array = (type*)sl_realloc                                                    \
       ((arr)->array, sizeof (type)*(arr)->capacity);                                   \
    memset(&((arr)->array)[old_idx], 0                                                  \
          , ((char*)&(((arr)->array)[idx])) - ((char*)&(((arr)->array)[old_idx])));     \
  }                                                                                     \
                                                                                        \
  (arr)->array[idx] = data;                                                             \
  if((arr)->length <= (idx))                                                            \
        (arr)->length = (idx) + 1;                                                      \
}while(0)


#define ARRAY_PUSH(type, arr, data)                                                    \
        ARRAY_PUT(type, arr, (arr)->length, data)

#define MOVE_MEM(type, dst, src, size)                                                 \
  memmove(dst, src, sizeof(type) * size)

#define MOVE_FAST(type, dst, src, size)                                                \
    memcpy((dst), (src) + (size), sizeof(type))

#define ARRAY_DEL(type, arr, idx, move)                                                \
if( (idx) < (arr)->length)                                                             \
{                                                                                      \
    move(type, &((arr)->array[idx]),                                                   \
    &((arr)->array[(idx)+1]), (arr)->length - (idx) - 1);                              \
    -- (arr)->length;                                                                  \
}

#define ARRAY_POP(type, arr) ARRAY_DEL(type, arr, 0, MOVE_MEM)

#endif
