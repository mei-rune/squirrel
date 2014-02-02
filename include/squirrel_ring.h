#ifndef _SRING_BUFFER_H_
#define _SRING_BUFFER_H_ 1

#include "squirrel_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RING_BUFFER_DEFINE(name, type)                                         \
typedef struct name##_s {                                                      \
  size_t      start;       /* index of oldest element              */          \
  size_t      count;       /* the count of elements                */          \
  type*       elements;    /* the vector of elements               */          \
  size_t      capacity;    /* the capacity of elements             */          \
} name##_t;                                                                    \
                                                                               \
static inline void name##_init(name##_t* self,                                 \
                                    type* elements, size_t capacity) {         \
  self->elements = elements;                                                   \
  self->capacity = capacity;                                                   \
  self->start = 0;                                                             \
  self->count = 0;                                                             \
}                                                                              \
                                                                               \
static inline boolean name##_is_full(name##_t* self) {                         \
  return self->count == self->capacity;                                        \
}                                                                              \
                                                                               \
static inline boolean name##_is_empty(name##_t* self) {                        \
  return 0 == self->count;                                                     \
}                                                                              \
                                                                               \
/* prepare an element for write, overwriting oldest element if buffer is full. \
   App can choose to avoid the overwrite by checking isFull(). */              \
static inline type* name##_prepare(name##_t* self) {                           \
  size_t end;                                                                  \
  type  *elem;                                                                 \
                                                                               \
  end = (self->start + self->count) % self->capacity;                          \
  elem = &self->elements[end];                                                 \
  if(self->count == self->capacity) {                                          \
    self->start = (self->start + 1) % self->capacity; /* full, overwrite */    \
    self->count--;                                                             \
  }                                                                            \
  return elem;                                                                 \
}                                                                              \
                                                                               \
static inline void name##_push(name##_t* self) {                               \
  if (self->count != self->capacity) {                                         \
    self->count++;                                                             \
  }                                                                            \
}                                                                              \
                                                                               \
/* Read oldest element. App must ensure !isEmpty() first. */                   \
static inline type* name##_pop(name##_t* self) {                               \
  type  *elem;                                                                 \
                                                                               \
  if(name##_is_empty(self)) {                                                  \
    return nil;                                                                \
  }                                                                            \
                                                                               \
  elem = &self->elements[self->start];                                         \
  self->start = (self->start + 1) % self->capacity;                            \
  self->count--;                                                               \
  return elem;                                                                 \
}                                                                              \
                                                                               \
static inline type* name##_get(name##_t* self, int idx) {                      \
  if(name##_is_empty(self)) {                                                  \
    return nil;                                                                \
  }                                                                            \
  return &self->elements[(self->start + idx) % self->capacity];                \
}                                                                              \
                                                                               \
static inline type* name##_first(name##_t* self) {                             \
  if(name##_is_empty(self)) {                                                  \
    return nil;                                                                \
  }                                                                            \
  return &self->elements[self->start];                                         \
}                                                                              \
                                                                               \
static inline type* name##_last(name##_t* self) {                              \
  if(name##_is_empty(self)) {                                                  \
    return nil;                                                                \
  }                                                                            \
                                                                               \
  return &self->elements[(self->start + self->count- 1) % self->capacity];     \
}                                                                              \
                                                                               \
static inline size_t name##_size(name##_t* self) {                             \
  return self->count;                                                          \
}                                                                              \
                                                                               \
/* clear all elements.*/                                                       \
static inline size_t name##_clear(name##_t* self) {                            \
  self->start = 0;                                                             \
  self->count= 0;                                                              \
}



/* Read all elements.*/
#define ring_buffer_foreach(idx, elem, self) for(idx =0,                       \
elem = &self->elements[(self->start) % self->capacity];                        \
idx < self->count;                                                             \
elem = &self->elements[(self->start + (++ idx) ) % self->capacity])

#ifdef __cplusplus
};
#endif

#endif //_SRING_BUFFER_H_