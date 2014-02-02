
#include "squirrel_config.h"
#include <assert.h>
#include "squirrel_hashtable.h"
#include "squirrel_string.h"

#ifdef __cplusplus
extern "C" {
#endif

#define up_stream_get_key(x) &((x)->name)
#define del_upstream(k, v)

typedef struct upstream_s {
  sstring_t name;
} upstream_t;


HASH_DECLARE_WITH_ENTRY(EMPTY, upstreams, sstring_t*, upstream_t*);
HASH_DEFINE_WITH_ENTRY(EMPTY, upstreams, sstring_t*, upstream_t*, up_stream_get_key, sstring_hash, sstring_cmp, del_upstream);


#ifdef __cplusplus
};
#endif