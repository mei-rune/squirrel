#ifndef _pthread_windows_h_
#define _pthread_windows_h_ 1


#include "squirrel_config.h"

#ifdef _WIN32
#include <process.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "squirrel_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PTHREAD_CANCEL_ENABLE        0x01  /* Cancel takes place at next cancellation point */
#define PTHREAD_CANCEL_DISABLE       0x00  /* Cancel postponed */
#define PTHREAD_CANCEL_DEFERRED      0x02  /* Cancel waits until cancellation point */

/**
 * @defgroup thread 线程相关的函数
 *  @{
 */
typedef HANDLE pthread_t;
typedef struct _opaque_pthread_attr_t {
  long __sig;
} pthread_attr_t;
typedef DWORD pthread_key_t;

DLL_VARIABLE int pthread_create(pthread_t * thread,
                                const pthread_attr_t * attr,
                                void *(*start_routine)(void *),
                                void * arg);


DLL_VARIABLE int pthread_detach (pthread_t tid);

DLL_VARIABLE int pthread_join(pthread_t thread, void **value_ptr);

DLL_VARIABLE void pthread_exit(void *value_ptr);

DLL_VARIABLE int pthread_attr_init(pthread_attr_t *attr);

DLL_VARIABLE int pthread_attr_destroy(pthread_attr_t *attr);

DLL_VARIABLE int pthread_setcancelstate(int state, int *oldstate);

DLL_VARIABLE pthread_t pthread_self(void);

DLL_VARIABLE int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));

DLL_VARIABLE int pthread_key_delete(pthread_key_t key);

DLL_VARIABLE int pthread_setspecific(pthread_key_t key, const void *value);

DLL_VARIABLE void * pthread_getspecific(pthread_key_t key);

/**
 *  @}
 */

/**
 * @defgroup mutex Mutex相关的函数
 *  @{
 */
typedef struct _opaque_pthread_mutexattr_t {
  long __sig;
} pthread_mutexattr_t;
typedef struct _opaque_pthread_condattr_t {
  long __sig;
} pthread_condattr_t;
typedef CRITICAL_SECTION pthread_mutex_t;

DLL_VARIABLE int pthread_mutex_init(pthread_mutex_t * mutex,
                                    const pthread_mutexattr_t  * attr);

DLL_VARIABLE int pthread_mutex_lock(pthread_mutex_t *mutex);

DLL_VARIABLE int pthread_mutex_unlock(pthread_mutex_t *mutex);

DLL_VARIABLE int pthread_mutex_destroy(pthread_mutex_t *mutex);

/**
 *  @}
 */

/**
 * @defgroup CONDITION_VARIABLE Condition variable相关的函数
 *  @{
 */
#if defined(_WIN32_WINNT_VISTA) && (_WIN32_WINNT >= _WIN32_WINNT_VISTA)

typedef CONDITION_VARIABLE pthread_cond_t;

#else

typedef struct {
  int waiters_count_;
  // Count of the number of waiters.

  CRITICAL_SECTION waiters_count_lock_;
  // Serialize access to <waiters_count_>.

  int release_count_;
  // Number of threads to release via a <pthread_cond_broadcast> or a
  // <pthread_cond_signal>.

  int wait_generation_count_;
  // Keeps track of the current "generation" so that we don't allow
  // one thread to steal all the "releases" from the broadcast.

  HANDLE event_;
  // A manual-reset event that's used to block and release waiting
  // threads.
} pthread_cond_t;

#endif

DLL_VARIABLE int pthread_cond_init(pthread_cond_t * cond, const pthread_condattr_t * attr);

DLL_VARIABLE int pthread_cond_signal(pthread_cond_t *cond);

DLL_VARIABLE int pthread_cond_broadcast(pthread_cond_t *cond);

DLL_VARIABLE int pthread_cond_wait(pthread_cond_t * cond, pthread_mutex_t  * mutex);

DLL_VARIABLE int pthread_cond_destroy(pthread_cond_t *cond);

/**
 *  @}
 */

/**
 * @defgroup once 相关的函数
 *  @{
 */

#define PTHREAD_ONCE_INIT       { 0, 0, 0, 0}

typedef struct pthread_once_t_ {
  long         done;        /* 指示用户的函数是否被执行过 */
  void *       lock;
  int          reserved1;
  int          reserved2;
} pthread_once_t;


DLL_VARIABLE int pthread_once (pthread_once_t * once_control, void (*init_routine) (void));

/**
 *  @}
 */

#ifdef __cplusplus
};
#endif

#endif // _WIN32

#endif // _pthread_windows_h_
