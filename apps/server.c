
#include "squirrel_config.h"
#include "server.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POLL_TIMEOUT    1000 *1000 * 1000


static int _upstream_poll(upstream_t* us, uint64_t timeout, request_ctx_t** res) {
  int            rc = 0;
  request_ctx_t  *v;
  uv_mutex_lock(&us->lock);
retry:
  if(0 != us->is_stopped) {
    rc = UV_EOF;
    goto end;
  }
  v = request_ctx_buffer_pop(&us->queue);
  if(nil == v) {
    rc = uv_cond_timedwait(&us->cond, &us->lock, timeout);
    if(0 == rc) {
      goto retry;
    }
  } else {
    *res = v;
  }
end:
  uv_mutex_unlock(&us->lock);
  return rc;
}

void _upstream_on_request(upstream_t* us, request_ctx_t  *res) {
}

void _upstream_on_timeout(upstream_t* us) {
}

void upstream_entry(void* act) {
  upstream_t     *us;
  request_ctx_t  *res;
  int             rc;
  int             is_async;
  int             is_free;

  us = (upstream_t*)act;
  for(;;) {
    rc = _upstream_poll(us, POLL_TIMEOUT, &res);
    switch (rc) {
    case 0:
      _upstream_on_request(us, res);
      break;
    case UV_ETIMEDOUT:
      _upstream_on_timeout(us);
      break;
    case UV_EOF:
      goto end;
    }
  }
end:
  uv_mutex_lock(&us->lock);
  is_async = us->is_async;
  is_free = us->is_free;
  uv_mutex_unlock(&us->lock);

  if(0 != is_async) {
    uv_mutex_destroy(&us->lock);
    uv_cond_destroy(&us->cond);

    sl_free(sstring_data(us->name));
    sl_free(us->queue.elements);
    if(0 != is_free) {
      sl_free(us);
    }
  }
}

int  upstream_init(upstream_t* us, const char* name, size_t name_len, size_t queue_capacity) {
  int rc;

  rc = uv_mutex_init(&us->lock);
  if(0 != rc) {
    ERR("uv_mutex_init: %s.", uv_strerror(rc));
    return rc;
  }
  rc = uv_cond_init(&us->cond);
  if(0 != rc) {
    uv_mutex_destroy(&us->lock);
    ERR("uv_cond_init: %s.", uv_strerror(rc));
    return rc;
  }

  sstring_init(us->name, sl_strdup(name), ((-1 == name_len)?strlen(name): name_len));
  if(nil == us->name.str) {
    uv_mutex_destroy(&us->lock);
    uv_cond_destroy(&us->cond);
    ERR("sl_malloc return nil.");
    return SHTTP_RES_MEMORY;
  }

  queue_capacity = ((-1 == queue_capacity)?100:queue_capacity);
  request_ctx_buffer_init(&us->queue,
                          (request_ctx_t*)sl_malloc(queue_capacity *sizeof(request_ctx_t)),
                          queue_capacity);
  rc = uv_thread_create(&us->tid, &upstream_entry, us);
  if(nil == us->name.str) {
    uv_mutex_destroy(&us->lock);
    uv_cond_destroy(&us->cond);
    ERR("uv_thread_create: %s.", uv_strerror(rc));
    return rc;
  }
  return 0;
}

int  upstream_send(upstream_t* us) {
  uv_mutex_lock(&us->lock);
  uv_mutex_unlock(&us->lock);
  return SHTTP_RES_NOTIMPLEMENT;
}

void upstream_destory(upstream_t* us, uint32_t async) {
  uv_mutex_lock(&us->lock);
  us->is_stopped = 1;
  us->is_async = (async & UPSTREAM_ASYNC);
  us->is_free = (async & UPSTREAM_FREE);
  uv_cond_signal(&us->cond);
  uv_mutex_unlock(&us->lock);
  if(0 != us->is_async) {
    return;
  }

  uv_thread_join(&us->tid);
  uv_mutex_destroy(&us->lock);
  uv_cond_destroy(&us->cond);

  sl_free(sstring_data(us->name));
  sl_free(us->queue.elements);
  if(0 != us->is_free) {
    sl_free(us);
  }
}

#ifdef __cplusplus
};
#endif