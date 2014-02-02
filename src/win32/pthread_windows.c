
#include "pthread_windows.h"

#ifdef _WIN32

#ifdef __cplusplus
extern "C" {
#endif

DLL_VARIABLE int pthread_create(pthread_t * thread,
                                const pthread_attr_t * attr,
                                void *(*start_routine)(void *),
                                void * arg) {
  *thread = (HANDLE)_beginthreadex(0, 0, (unsigned(__stdcall*)(void*))start_routine, arg, 0, 0);
  return 0;
}

DLL_VARIABLE int pthread_join(pthread_t thread, void **value_ptr) {
  LPDWORD exitcode = 0;
  WaitForSingleObject(thread, INFINITE);
  if (value_ptr)  {
    GetExitCodeThread(thread,exitcode);
    *value_ptr = exitcode;
  }
  CloseHandle(thread);
  return 0;
}


DLL_VARIABLE int pthread_detach (pthread_t thread) {
  return CloseHandle(thread)?0:-1;
}

DLL_VARIABLE void pthread_exit(void *value_ptr) {
#pragma warning( disable : 4311 )
  _endthreadex((DWORD)value_ptr);
#pragma warning( default : 4311 )
}

DLL_VARIABLE int pthread_attr_init(pthread_attr_t *attr) {
  // do nothing currently
  return 0;
}

DLL_VARIABLE int pthread_attr_destroy(pthread_attr_t *attr) {
  // do nothing currently
  return 0;
}

DLL_VARIABLE int pthread_setcancelstate(int state, int *oldstate) {
  // do nothing currently
  return 0;
}

DLL_VARIABLE int pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t  * attr) {
  if (attr) return(EINVAL);

#if (_WIN32_WINNT >= 0x0403)
  return (TRUE == InitializeCriticalSectionAndSpinCount(mutex, 1500))?0:-1;
#else
  InitializeCriticalSection(mutex);
  return 0;
#endif
}

DLL_VARIABLE int pthread_mutex_lock(pthread_mutex_t *mutex) {
  EnterCriticalSection(mutex);
  return 0;
}

DLL_VARIABLE int pthread_mutex_unlock(pthread_mutex_t *mutex) {
  LeaveCriticalSection(mutex);
  return 0;
}

DLL_VARIABLE int pthread_mutex_destroy(pthread_mutex_t *mutex) {
  DeleteCriticalSection(mutex);
  return 0;
}


DLL_VARIABLE pthread_t pthread_self(void) {
  return(GetCurrentThread());
}

DLL_VARIABLE int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
  *key = TlsAlloc();
  return (TLS_OUT_OF_INDEXES == *key)?-1:0;
}

DLL_VARIABLE int pthread_key_delete(pthread_key_t key) {
  TlsFree(key);
  return 0;
}

DLL_VARIABLE int pthread_setspecific(pthread_key_t key, const void *value) {
  TlsSetValue(key, (LPVOID) value);
  return 0;
}

DLL_VARIABLE void * pthread_getspecific(pthread_key_t key) {
  return TlsGetValue(key);
}

#if defined(_WIN32_WINNT_VISTA) && (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
DLL_VARIABLE int pthread_cond_init(pthread_cond_t  * cond,
                                   const pthread_condattr_t  * attr) {
  if (attr) return(EINVAL);
  InitializeConditionVariable(cond);
  return 0;
}

DLL_VARIABLE int pthread_cond_signal(pthread_cond_t *cond) {
  WakeConditionVariable(cond);
  return 0;// Errors not managed
}

DLL_VARIABLE int pthread_cond_broadcast(pthread_cond_t *cond) {
  WakeAllConditionVariable(cond);
  return(0);
}

DLL_VARIABLE int pthread_cond_wait(pthread_cond_t  * cond,
                                   pthread_mutex_t  * mutex) {
  SleepConditionVariableCS(cond, mutex, INFINITE);
  return 0;// Errors not managed
}

DLL_VARIABLE int pthread_cond_destroy(pthread_cond_t *cond) {
  return 0;
}
#else

DLL_VARIABLE int pthread_cond_init(pthread_cond_t * cv,
                                   const pthread_condattr_t * attr) {
  cv->waiters_count_ = 0;
  cv->wait_generation_count_ = 0;
  cv->release_count_ = 0;

  cv->event_ = CreateEvent (NULL,  // no security
                            TRUE,  // manual-reset
                            FALSE, // non-signaled initially
                            NULL); // unnamed

  pthread_mutex_init(&cv->waiters_count_lock_,NULL);
  return 0;
}

DLL_VARIABLE int pthread_cond_wait(pthread_cond_t * cv,
                                   pthread_mutex_t * external_mutex) {
  int last_waiter = 0;
  int my_generation = 0;

  // Avoid race conditions.
  EnterCriticalSection (&cv->waiters_count_lock_);

  // Increment count of waiters.
  cv->waiters_count_++;

  // Store current generation in our activation record.
  my_generation = cv->wait_generation_count_;

  LeaveCriticalSection (&cv->waiters_count_lock_);
  LeaveCriticalSection (external_mutex);

  for (;;) {
    int wait_done = 0;
    // Wait until the event is signaled.
    WaitForSingleObject (cv->event_, INFINITE);

    EnterCriticalSection (&cv->waiters_count_lock_);
    // Exit the loop when the <cv->event_> is signaled and
    // there are still waiting threads from this <wait_generation>
    // that haven't been released from this wait yet.
    wait_done = cv->release_count_ > 0
                && cv->wait_generation_count_ != my_generation;
    LeaveCriticalSection (&cv->waiters_count_lock_);

    if (wait_done)
      break;
  }

  EnterCriticalSection (external_mutex);
  EnterCriticalSection (&cv->waiters_count_lock_);
  cv->waiters_count_--;
  cv->release_count_--;
  last_waiter = cv->release_count_ == 0;
  LeaveCriticalSection (&cv->waiters_count_lock_);

  if (last_waiter)
    // We're the last waiter to be notified, so reset the manual event.
    ResetEvent (cv->event_);
  return 0;
}

DLL_VARIABLE int pthread_cond_signal(pthread_cond_t *cv) {
  EnterCriticalSection (&cv->waiters_count_lock_);
  if (cv->waiters_count_ > cv->release_count_) {
    SetEvent (cv->event_); // Signal the manual-reset event.
    cv->release_count_++;
    cv->wait_generation_count_++;
  }
  LeaveCriticalSection (&cv->waiters_count_lock_);
  return 0;
}

DLL_VARIABLE int pthread_cond_broadcast(pthread_cond_t *cv) {
  EnterCriticalSection (&cv->waiters_count_lock_);
  if (cv->waiters_count_ > 0) {
    SetEvent (cv->event_);
    // Release all the threads in this generation.
    cv->release_count_ = cv->waiters_count_;

    // Start a new generation.
    cv->wait_generation_count_++;
  }
  LeaveCriticalSection (&cv->waiters_count_lock_);
  return 0;
}

DLL_VARIABLE int pthread_cond_destroy(pthread_cond_t *cond) {
  DeleteCriticalSection(&cond->waiters_count_lock_);
  CloseHandle(cond->event_);
  return (0); // Errors not managed
}

#endif

DLL_VARIABLE int pthread_once (pthread_once_t * once_control, void (*init_routine) (void)) {
  if (once_control == NULL || init_routine == NULL)
    return EINVAL;

#ifndef _WIN32
  if(0 == __sync_val_compare_and_swap(&once_control->done,0, 1))
#else
  if(0 == InterlockedCompareExchange(&once_control->done,1, 0))
#endif
  {
    (*init_routine)();

#ifndef _WIN32
    __sync_val_compare_and_swap(&once_control->done, 1, 2);
#else
    InterlockedExchange(&once_control->done,2);
#endif

  } else {
    // 等待直到执行完成
#ifndef _WIN32
    while(2 != __sync_val_compare_and_swap(&once_control->done,2, 2))
#else
    while(2 != InterlockedCompareExchange(&once_control->done,2, 2))
#endif
      Sleep(10);
  }

  return 0;

}


#ifdef __cplusplus
};
#endif

#endif // _FF_MINPORT_WIN_H_
