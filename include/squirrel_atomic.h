
#ifndef _squirrel_atomic_h
#define _squirrel_atomic_h 1

#include "squirrel_config.h"


#ifdef __cplusplus
extern "C" {
#endif


#ifdef _WIN32
#define atomic32_t LONG volatile
#define atomic_add32(mem, val)               InterlockedAdd(mem, val)
#define atomic_sub32(mem, val)               InterlockedAdd(mem , - (val))
#define atomic_dec32(mem)                    InterlockedDecrement(mem)
#define atomic_inc32(mem)                    InterlockedIncrement(mem)
#define atomic_read32(mem)                   InterlockedAdd(mem, 0)
#define atomic_set32(mem, val)               InterlockedExchange(mem , val)
#define atomic_cvs32(mem, new_val, old_val)  InterlockedCompareExchange(mem, new_val, old_val)
#else
#define atomic32_t int volatile
#define atomic_add32(mem, val)               __sync_add_and_fetch (mem, val)
#define atomic_sub32(mem, val)               __sync_sub_and_fetch (mem, val)
#define atomic_dec32(mem)                    __sync_sub_and_fetch(mem, 1)
#define atomic_inc32(mem)                    __sync_add_and_fetch(mem, 1)
#define atomic_read32(mem)                   __sync_add_and_fetch(mem, 0)
#define atomic_set32(mem, val)               __sync_lock_test_and_set(mem, val)
#define atomic_cvs32(mem, new_val, old_val)  __sync_val_compare_and_swap(mem,new_val, old_val))
#endif

#ifdef __cplusplus
};
#endif

#endif    /** _squirrel_atomic_h */
