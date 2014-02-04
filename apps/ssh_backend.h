
#include "squirrel_config.h"
#include <assert.h>
#include "uv.h"
#include "squirrel_hashtable.h"
#include "squirrel_string.h"
#include "squirrel_ring.h"
#include "squirrel_http.h"

#ifdef __cplusplus
extern "C" {
#endif

#define up_stream_get_key(x)    (&((x)->name))
#define del_upstream(k, v)      upstream_destory(v, UPSTREAM_ASYNC|UPSTREAM_FREE)

typedef struct request_ctx_s {
  shttp_connection_t *conn;
} request_ctx_t;

RING_BUFFER_DEFINE(request_ctx_buffer, request_ctx_t);

typedef struct upstream_s {
  uv_mutex_t            lock;
  uv_cond_t             cond;
  uv_thread_t           tid;
  uint32_t              is_stopped:1;
  uint32_t              is_async:1;
  uint32_t              is_free:1;
  uint32_t              resvered:29;

  sstring_t             name;
  request_ctx_buffer_t  queue;
} upstream_t;

int  upstream_init(upstream_t* us, const char* name, size_t name_len);
int  upstream_send(upstream_t* us);
#define UPSTREAM_ASYNC    1
#define UPSTREAM_FREE     2
void upstream_destory(upstream_t* us, uint32_t flags);

#define upstream_send_prepare_lock(us) uv_mutex_lock(&us->lock)
#define upstream_buffer_is_empty(us) request_ctx_buffer_is_empty(&us->queue)
#define upstream_send_prepare(us) request_ctx_buffer_prepare(&us->queue)
#define upstream_send_step(us) request_ctx_buffer_step(&us->queue)
#define upstream_send_prepare_unlock(us)  uv_mutex_unlock(&us->lock)

HASH_DECLARE_WITH_ENTRY(EMPTY, upstreams, sstring_t*, upstream_t*);
HASH_DEFINE_WITH_ENTRY(EMPTY, upstreams, sstring_t*, upstream_t*,
                       up_stream_get_key, sstring_hash, sstring_cmp, del_upstream);



typedef struct server_s {
  upstreams_t    upstreams;
} server_t;

#ifdef __cplusplus
};
#endif