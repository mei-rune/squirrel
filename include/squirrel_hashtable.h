#ifndef _hashtable_h_
#define _hashtable_h_ 1

#include <stdlib.h>

#define HASHTABLE_MAX_USAGE 0.60

#define HASH_DECLARE_THUNK(name, key_type, value_type)                                              \
typedef struct name##_entry_s {                                                                     \
  key_type key;                                                                                     \
  value_type value;                                                                                 \
} name##_entry_t;                                                                                   \
                                                                                                    \
                                                                                                    \
typedef struct name##_thunk_s {                                                                     \
  name##_entry_t  ref;                                                                              \
  struct name##_thunk_s* next;                                                                      \
} name##_thunk_t;


#define HASH_DECLARE_TABLE(name, thunk_type, key_type, value_type)                                  \
typedef struct name##_s {                                                                           \
  size_t size;                                                                                      \
  size_t used;                                                                                      \
  thunk_type** entries;                                                                             \
} name##_t;



#define HASH_DECLARE_ITERATOR(name, thunk_type, key_type, value_type)                               \
typedef struct name##_iterator_s {                                                                  \
  name##_t* table;                                                                                  \
  size_t index;                                                                                     \
  thunk_type* next;                                                                                 \
} name##_iterator_t;



#define HASH_DECLARE_FUNC(FUNC_DECL, name, key_type, value_type)                                    \
FUNC_DECL    void name##_init(name##_t* instance, size_t default_size);                             \
FUNC_DECL    void name##_destroy(name##_t* instance);                                               \
FUNC_DECL    void name##_clear(name##_t* instance);                                                 \
FUNC_DECL    value_type name##_at(const name##_t* instance, size_t idx, value_type defaultValue);   \
FUNC_DECL    value_type name##_get(const name##_t* instance, key_type key, value_type defaultValue);\
FUNC_DECL    void name##_put(name##_t* instance, key_type key, value_type val);                     \
FUNC_DECL    boolean name##_del(name##_t* instance, key_type key);                                  \
FUNC_DECL    size_t name##_count(name##_t* instance);


#define HASH_DECLARE_FUNC_ITERATOR(FUNC_DECL, name, key_type, value_type)                           \
FUNC_DECL    void name##_iterator_init(name##_iterator_t* it, name##_t* instance);                  \
FUNC_DECL    void name##_iterator_destroy(name##_iterator_t* it);                                   \
FUNC_DECL    boolean name##_iterator_next(name##_iterator_t* it);                                   \
FUNC_DECL    name##_entry_t* name##_iterator_current(name##_iterator_t* it);


#define HASH_DECLARE(FUNC_DECL, name, key_type, value_type)                                        \
  HASH_DECLARE_THUNK(name, key_type, value_type)                                                   \
  HASH_DECLARE_TABLE(name, name##_thunk_t, key_type, value_type)                                   \
  HASH_DECLARE_ITERATOR(name, name##_thunk_t, key_type, value_type)                                \
  HASH_DECLARE_FUNC(FUNC_DECL, name, key_type, value_type)                                         \
  HASH_DECLARE_FUNC_ITERATOR(FUNC_DECL, name, key_type, value_type)

#define HASH_DECLARE2(FUNC_DECL, name, thunk_type, key_type, value_type)                           \
  HASH_DECLARE_TABLE(name, thunk_type, key_type, value_type)                                       \
  HASH_DECLARE_FUNC(FUNC_DECL, name, key_type, value_type)


#define HASH_DEFINE2(FUNC_DECL                                                                     \
    , name                                                                                         \
    , thunk_type                                                                                   \
    , key_type                                                                                     \
    , value_type                                                                                   \
    , get_key                                                                                      \
    , get_value                                                                                    \
    , get_next                                                                                     \
    , set_key                                                                                      \
    , set_value                                                                                    \
    , set_next                                                                                     \
    , hash                                                                                         \
    , cmp                                                                                          \
    , del)                                                                                         \
                                                                                                   \
void _##name##_resize(name##_t* instance);                                                         \
                                                                                                   \
FUNC_DECL    void name##_init(name##_t* instance, size_t default_size) {                           \
  instance->size = default_size;                                                                   \
  instance->used = 0;                                                                              \
                                                                                                   \
  instance->entries = (thunk_type**)sl_calloc(instance->size, sizeof(thunk_type*));                \
}                                                                                                  \
                                                                                                   \
FUNC_DECL    void name##_destroy(name##_t* instance) {                                             \
  if(0 == instance) {                                                                              \
    return;                                                                                        \
  }                                                                                                \
  name##_clear(instance);                                                                          \
  sl_free(instance->entries);                                                                      \
}                                                                                                  \
                                                                                                   \
void _##name##_put(name##_t* instance, key_type key, value_type val, thunk_type* new_el) {         \
                                                                                                   \
  thunk_type* el = NULL;                                                                           \
  thunk_type* parent_el = NULL;                                                                    \
                                                                                                   \
  size_t  index = hash(key) % instance->size;                                                      \
  thunk_type*  root_el = instance->entries[index];                                                 \
                                                                                                   \
  el = root_el;                                                                                    \
  while (el && cmp(get_key(el), key)) {                                                            \
    parent_el = el;                                                                                \
    el = get_next(el);                                                                             \
  }                                                                                                \
                                                                                                   \
  if(el) {                                                                                         \
    assert(0 == new_el);                                                                           \
                                                                                                   \
    del((get_key(el) == key)?NULL:get_key(el)                                                      \
      , (get_value(el) == val)?NULL:get_value(el));                                                \
                                                                                                   \
    set_key(el, key);                                                                              \
    set_value(el, val);                                                                            \
    return;                                                                                        \
  }                                                                                                \
                                                                                                   \
  if(0 == new_el) {                                                                                \
    new_el = (thunk_type*) sl_malloc(sizeof(thunk_type));                                          \
    set_key(new_el, key);                                                                          \
    set_value(new_el, val);                                                                        \
  }                                                                                                \
                                                                                                   \
  set_next(new_el, NULL);                                                                          \
                                                                                                   \
  if (parent_el) {                                                                                 \
    set_next(parent_el, new_el);                                                                   \
  } else  {                                                                                        \
    instance->entries[index] = new_el;                                                             \
  }                                                                                                \
                                                                                                   \
  instance->used++;                                                                                \
                                                                                                   \
  if ((double) instance->used / instance->size > HASHTABLE_MAX_USAGE) {                            \
    _##name##_resize(instance);                                                                    \
  }                                                                                                \
  return;                                                                                          \
}                                                                                                  \
                                                                                                   \
void _##name##_resize(name##_t* instance) {                                                        \
                                                                                                   \
  size_t old_size  = instance->size;                                                               \
  thunk_type** old_entries = instance->entries;                                                    \
                                                                                                   \
  instance->size *= 2;                                                                             \
  instance->entries = (thunk_type**)sl_calloc(instance->size, sizeof(thunk_type*));                \
                                                                                                   \
  instance->used = 0;                                                                              \
                                                                                                   \
  {                                                                                                \
    size_t i;                                                                                      \
    for (i = 0; i < old_size; i++) {                                                               \
      thunk_type* el = old_entries[i];                                                             \
      while (el) {                                                                                 \
        thunk_type* next = get_next(el);                                                           \
        _##name##_put(instance, get_key(el), get_value(el), el);                                   \
        el = next;                                                                                 \
      }                                                                                            \
    }                                                                                              \
  }                                                                                                \
                                                                                                   \
  sl_free(old_entries);                                                                            \
}                                                                                                  \
                                                                                                   \
FUNC_DECL    void name##_put(name##_t* instance, key_type key, value_type val) {                   \
                                                                                                   \
  if(0 == val) {                                                                                   \
    name##_del(instance, key);                                                                     \
  } else {                                                                                         \
    _##name##_put(instance, key, val, NULL);                                                       \
  }                                                                                                \
}                                                                                                  \
                                                                                                   \
FUNC_DECL    boolean name##_del(name##_t* instance, key_type key) {                                \
                                                                                                   \
  thunk_type* el = NULL;                                                                           \
  thunk_type* parent_el = NULL;                                                                    \
                                                                                                   \
  size_t  index = hash(key) % instance->size;                                                      \
  thunk_type*  root_el = instance->entries[index];                                                 \
                                                                                                   \
  el = root_el;                                                                                    \
  while (el && cmp(get_key(el), key)) {                                                            \
    parent_el = el;                                                                                \
    el = get_next(el);                                                                             \
  }                                                                                                \
                                                                                                   \
  if(!el) {  /* ������, �򷵻� */                                                                  \
    return false;                                                                                  \
  }                                                                                                \
                                                                                                   \
  if (parent_el) {                                                                                 \
    set_next(parent_el, get_next(el));                                                             \
  } else {                                                                                         \
    instance->entries[index] = get_next(el);                                                       \
  }                                                                                                \
  del(get_key(el), get_value(el));                                                                 \
                                                                                                   \
  sl_free(el);                                                                                     \
  instance->used--;                                                                                \
  return true;                                                                                     \
}                                                                                                  \
                                                                                                   \
FUNC_DECL    value_type name##_get(const name##_t* instance,                                       \
  key_type key, value_type defaultValue) {                                                         \
  size_t index = hash(key) % instance->size;                                                       \
  thunk_type* el = instance->entries[index];                                                       \
  while (el) {                                                                                     \
    if (cmp(get_key(el), key) == 0) {                                                              \
      return get_value(el);                                                                        \
    }                                                                                              \
    el = get_next(el);                                                                             \
  }                                                                                                \
                                                                                                   \
  return defaultValue;                                                                             \
}                                                                                                  \
                                                                                                   \
FUNC_DECL    value_type name##_at(const name##_t* instance,                                        \
  size_t idx, value_type defaultValue) {                                                           \
                                                                                                   \
  size_t i;                                                                                        \
  size_t count;                                                                                    \
  if(NULL == instance) {                                                                           \
    return defaultValue;                                                                           \
  }                                                                                                \
                                                                                                   \
  count = 0;                                                                                       \
  for (i = 0; i < instance->size; i++) {                                                           \
    thunk_type* el = instance->entries[i];                                                         \
    while (el) {                                                                                   \
      if(idx == count ++) {                                                                        \
        return get_value(el);                                                                      \
      }                                                                                            \
      el = get_next(el);                                                                           \
    }                                                                                              \
  }                                                                                                \
  return defaultValue;                                                                             \
}                                                                                                  \
                                                                                                   \
FUNC_DECL    void name##_clear(name##_t* instance) {                                               \
  size_t i;                                                                                        \
  if(0 == instance) {                                                                              \
    return;                                                                                        \
  }                                                                                                \
                                                                                                   \
  for (i = 0; i < instance->size; i++) {                                                           \
    thunk_type* el = instance->entries[i];                                                         \
    instance->entries[i] = 0;                                                                      \
    while (el) {                                                                                   \
      thunk_type* next = get_next(el);                                                             \
      del(get_key(el), get_value(el));                                                             \
      sl_free(el);                                                                                 \
      el = next;                                                                                   \
    }                                                                                              \
  }                                                                                                \
                                                                                                   \
  instance->used = 0;                                                                              \
}                                                                                                  \
                                                                                                   \
FUNC_DECL    size_t name##_count(name##_t* instance) {                                             \
  return instance->used;                                                                           \
}

#define HASH_DEFINE_ITERATOR(FUNC_DECL, name)                                                      \
                                                                                                   \
/** ����һ�� instance �ĵ����� */                                                                  \
FUNC_DECL    void name##_iterator_init(name##_iterator_t* it, name##_t* instance) {                \
  it->table = instance;                                                                            \
  it->index = -1;                                                                                  \
  it->next = NULL;                                                                                 \
}                                                                                                  \
                                                                                                   \
/** ����������ǰ��һλ */                                                                          \
FUNC_DECL    boolean name##_iterator_next(name##_iterator_t* it) {                                 \
                                                                                                   \
  if(NULL != it->next) {                                                                           \
    it->next = it->next->next;                                                                     \
  }                                                                                                \
  if(NULL != it->next) {                                                                           \
    return true;                                                                                   \
  }                                                                                                \
                                                                                                   \
  while( (++it->index) < it->table->size ) {                                                       \
    it->next = it->table->entries[it->index];                                                      \
    if(NULL != it->next) {                                                                         \
      return true;                                                                                 \
    }                                                                                              \
  }                                                                                                \
                                                                                                   \
  return false;                                                                                    \
}                                                                                                  \
                                                                                                   \
/** ��ȡ�������ĵ�ǰ���� */                                                                        \
FUNC_DECL    name##_entry_t* name##_iterator_current(name##_iterator_t* it) {                      \
  return (NULL != it->next)?&(it->next->ref):NULL;                                                 \
}


#define default_get_key(el) el->ref.key
#define default_get_value(el) el->ref.value
#define default_get_next(el) el->next


#define default_set_key(el, v) el->ref.key = v
#define default_set_value(el, v) el->ref.value = v
#define default_set_next(el, v) el->next = v

#define HASH_DEFINE(FUNC_DECL, name, key_type, value_type, hash, cmp, del)                         \
 HASH_DEFINE2(FUNC_DECL, name, name##_thunk_t, key_type, value_type                                \
            , default_get_key, default_get_value, default_get_next                                 \
            , default_set_key, default_set_value, default_set_next, hash, cmp, del)                \
 HASH_DEFINE_ITERATOR(FUNC_DECL, name)


#endif