

typedef struct int2int_entry_s {
  int key;
  int value;
} int2int_entry_t;
typedef struct int2int_thunk_s {
  int2int_entry_t ref;
  struct int2int_thunk_s* next;
} int2int_thunk_t;
typedef struct int2int_s {
  size_t size;
  size_t used;
  int2int_thunk_t** entries;
} int2int_t;
typedef struct int2int_iterator_s {
  int2int_t* table;
  size_t index;
  int2int_thunk_t* next;
} int2int_iterator_t;
void int2int_init(int2int_t* instance, size_t default_size);
void int2int_destroy(int2int_t* instance);
void int2int_clear(int2int_t* instance);
int int2int_at(const int2int_t* instance, size_t idx, int defaultValue);
int int2int_get(const int2int_t* instance, int key, int defaultValue);
void int2int_put(int2int_t* instance, int key, int val);
boolean int2int_del(int2int_t* instance, int key);
size_t int2int_count(int2int_t* instance);
void int2int_iterator_init(int2int_iterator_t* it, int2int_t* instance);
void int2int_iterator_destroy(int2int_iterator_t* it);
boolean int2int_iterator_next(int2int_iterator_t* it);
int2int_entry_t* int2int_iterator_current(int2int_iterator_t* it);;
void _int2int_resize(int2int_t* instance);
void int2int_init(int2int_t* instance, size_t default_size) {
  instance->size = default_size;
  instance->used = 0;
  instance->entries = (int2int_thunk_t**)calloc(instance->size, sizeof(int2int_thunk_t*));
}
void int2int_destroy(int2int_t* instance) {
  if(0 == instance) {
    return;
  }
  int2int_clear(instance);
  if(0 != instance->entries) free(instance->entries);
}
void _int2int_put(int2int_t* instance, int key, int val, int2int_thunk_t* new_el) {
  int2int_thunk_t* el = 0;
  int2int_thunk_t* parent_el = 0;
  size_t index = key % instance->size;
  int2int_thunk_t* root_el = instance->entries[index];
  el = root_el;
  while (el && (((el)->ref).key - key)) {
    parent_el = el;
    el = (el)->next;
  }
  if(el) {
    (void)( (!!(0 == new_el)) || (_wassert(L"0 == new_el", L"test\\hashtable_test.cpp", 10), 0) ); ;
    ((el)->ref).key = key;
    ((el)->ref).value = val;
    return;
  }
  if(0 == new_el) {
    new_el = (int2int_thunk_t*) malloc(sizeof(int2int_thunk_t));
    (new_el).key = key;
    (new_el).value = val;
  } (new_el)->next = 0;
  if (parent_el) {
    (parent_el)->next = new_el;
  } else {
    instance->entries[index] = new_el;
  }
  instance->used++;
  if ((double) instance->used / instance->size > 0.60) {
    _int2int_resize(instance);
  }
  return;
}
void _int2int_resize(int2int_t* instance) {
  size_t old_size = instance->size;
  int2int_thunk_t** old_entries = instance->entries;
  instance->size *= 2;
  instance->entries = (int2int_thunk_t**)calloc(instance->size, sizeof(int2int_thunk_t*));
  instance->used = 0;
  {
    size_t i;
    for (i = 0; i < old_size; i++) {
      int2int_thunk_t* el = old_entries[i];
      while (el) {
        int2int_thunk_t* next = (el)->next;
        _int2int_put(instance, ((el)->ref).key, ((el)->ref).value, el);
        el = next;
      }
    }
  }
  if(0 != old_entries) free(old_entries);
}
void int2int_put(int2int_t* instance, int key, int val) {
  if(0 == val) {
    int2int_del(instance, key);
  } else {
    _int2int_put(instance, key, val, 0);
  }
}
boolean int2int_del(int2int_t* instance, int key) {
  int2int_thunk_t* el = 0;
  int2int_thunk_t* parent_el = 0;
  size_t index = key % instance->size;
  int2int_thunk_t* root_el = instance->entries[index];
  el = root_el;
  while (el && (((el)->ref).key - key)) {
    parent_el = el;
    el = (el)->next;
  }
  if(!el) {
    return ((boolean)0);
  }
  if (parent_el) {
    (parent_el)->next = (el)->next;
  } else {
    instance->entries[index] = (el)->next;
  } ;
  if(0 != el) free(el);
  instance->used--;
  return ((boolean)1);
}
int int2int_get(const int2int_t* instance, int key, int defaultValue) {
  size_t index = key % instance->size;
  int2int_thunk_t* el = instance->entries[index];
  while (el) {
    if ((((el)->ref).key - key) == 0) {
      return ((el)->ref).value;
    }
    el = (el)->next;
  }
  return defaultValue;
}
int int2int_at(const int2int_t* instance, size_t idx, int defaultValue) {
  size_t i;
  size_t count;
  if(0 == instance) {
    return defaultValue;
  }
  count = 0;
  for (i = 0; i < instance->size; i++) {
    int2int_thunk_t* el = instance->entries[i];
    while (el) {
      if(idx == count ++) {
        return ((el)->ref).value;
      }
      el = (el)->next;
    }
  }
  return defaultValue;
}
void int2int_clear(int2int_t* instance) {
  size_t i;
  if(0 == instance) {
    return;
  }
  for (i = 0; i < instance->size; i++) {
    int2int_thunk_t* el = instance->entries[i];
    instance->entries[i] = 0;
    while (el) {
      int2int_thunk_t* next = (el)->next; ;
      if(0 != el) free(el);
      el = next;
    }
  }
  instance->used = 0;
}
size_t int2int_count(int2int_t* instance) {
  return instance->used;
}
void int2int_iterator_init(int2int_iterator_t* it, int2int_t* instance) {
  it->table = instance;
  it->index = -1;
  it->next = 0;
}
boolean int2int_iterator_next(int2int_iterator_t* it) {
  if(0 != it->next) {
    it->next = it->next->next;
  }
  if(0 != it->next) {
    return ((boolean)1);
  }
  while( (++it->index) < it->table->size ) {
    it->next = it->table->entries[it->index];
    if(0 != it->next) {
      return ((boolean)1);
    }
  }
  return ((boolean)0);
}
int2int_entry_t* int2int_iterator_current(int2int_iterator_t* it) {
  return (0 != it->next)?&(it->next->ref):0;
};

typedef struct int_thunk_s {
  int x;
  int y;
  struct int_thunk_s* aa;
} int_thunk_t;











typedef struct int_table_s {
  size_t size;
  size_t used;
  int_thunk_t** entries;
} int_table_t;
void int_table_init(int_table_t* instance, size_t default_size);
void int_table_destroy(int_table_t* instance);
void int_table_clear(int_table_t* instance);
int int_table_at(const int_table_t* instance, size_t idx, int defaultValue);
int int_table_get(const int_table_t* instance, int key, int defaultValue);
void int_table_put(int_table_t* instance, int key, int val);
boolean int_table_del(int_table_t* instance, int key);
size_t int_table_count(int_table_t* instance);;
void _int_table_resize(int_table_t* instance);
void int_table_init(int_table_t* instance, size_t default_size) {
  instance->size = default_size;
  instance->used = 0;
  instance->entries = (int_thunk_t**)calloc(instance->size, sizeof(int_thunk_t*));
}
void int_table_destroy(int_table_t* instance) {
  if(0 == instance) {
    return;
  }
  int_table_clear(instance);
  if(0 != instance->entries) free(instance->entries);
}
void _int_table_put(int_table_t* instance, int key, int val, int_thunk_t* new_el) {
  int_thunk_t* el = 0;
  int_thunk_t* parent_el = 0;
  size_t index = key % instance->size;
  int_thunk_t* root_el = instance->entries[index];
  el = root_el;
  while (el && (el->x - key)) {
    parent_el = el;
    el = el->aa;
  }
  if(el) {
    (void)( (!!(0 == new_el)) || (_wassert(L"0 == new_el", L"test\\hashtable_test.cpp", 42), 0) ); ;
    el->x = key;
    el->y = val;
    return;
  }
  if(0 == new_el) {
    new_el = (int_thunk_t*) malloc(sizeof(int_thunk_t));
    new_el->x = key;
    new_el->y = val;
  }
  new_el->aa = 0;
  if (parent_el) {
    parent_el->aa = new_el;
  } else {
    instance->entries[index] = new_el;
  }
  instance->used++;
  if ((double) instance->used / instance->size > 0.60) {
    _int_table_resize(instance);
  }
  return;
}
void _int_table_resize(int_table_t* instance) {
  size_t old_size = instance->size;
  int_thunk_t** old_entries = instance->entries;
  instance->size *= 2;
  instance->entries = (int_thunk_t**)calloc(instance->size, sizeof(int_thunk_t*));
  instance->used = 0;
  {
    size_t i;
    for (i = 0; i < old_size; i++) {
      int_thunk_t* el = old_entries[i];
      while (el) {
        int_thunk_t* next = el->aa;
        _int_table_put(instance, el->x, el->y, el);
        el = next;
      }
    }
  }
  if(0 != old_entries) free(old_entries);
}
void int_table_put(int_table_t* instance, int key, int val) {
  if(0 == val) {
    int_table_del(instance, key);
  } else {
    _int_table_put(instance, key, val, 0);
  }
}
boolean int_table_del(int_table_t* instance, int key) {
  int_thunk_t* el = 0;
  int_thunk_t* parent_el = 0;
  size_t index = key % instance->size;
  int_thunk_t* root_el = instance->entries[index];
  el = root_el;
  while (el && (el->x - key)) {
    parent_el = el;
    el = el->aa;
  }
  if(!el) {
    return ((boolean)0);
  }
  if (parent_el) {
    parent_el->aa = el->aa;
  } else {
    instance->entries[index] = el->aa;
  } ;
  if(0 != el) free(el);
  instance->used--;
  return ((boolean)1);
}
int int_table_get(const int_table_t* instance, int key, int defaultValue) {
  size_t index = key % instance->size;
  int_thunk_t* el = instance->entries[index];
  while (el) {
    if ((el->x - key) == 0) {
      return el->y;
    }
    el = el->aa;
  }
  return defaultValue;
}
int int_table_at(const int_table_t* instance, size_t idx, int defaultValue) {
  size_t i;
  size_t count;
  if(0 == instance) {
    return defaultValue;
  }
  count = 0;
  for (i = 0; i < instance->size; i++) {
    int_thunk_t* el = instance->entries[i];
    while (el) {
      if(idx == count ++) {
        return el->y;
      }
      el = el->aa;
    }
  }
  return defaultValue;
}
void int_table_clear(int_table_t* instance) {
  size_t i;
  if(0 == instance) {
    return;
  }
  for (i = 0; i < instance->size; i++) {
    int_thunk_t* el = instance->entries[i];
    instance->entries[i] = 0;
    while (el) {
      int_thunk_t* next = el->aa; ;
      if(0 != el) free(el);
      el = next;
    }
  }
  instance->used = 0;
}
size_t int_table_count(int_table_t* instance) {
  return instance->used;
} ;
#line 43 "test\\hashtable_test.cpp"


void __cdecl test_hashtable_simple_run(out_fn_t out_fn);
int test_hashtable_simple_var = ADD_RUN_TEST("hashtable_simple" , &test_hashtable_simple_run);
void __cdecl test_hashtable_simple_run(out_fn_t out_fn) {
  int2int_t h;
  int2int_init(&h, 3);
  int2int_put(&h, 1,1);
  do {
    if (!((1) == (int2int_get(&h, 1, 0)))) {
      do {
        if(0 != out_fn) {
          char buf[600];
          size_t len = _snprintf(buf, sizeof(buf), "Check failed: " "1" "==" "int2int_get(&h, 1, 0)" " at %s, %d\n", "test\\hashtable_test.cpp", 49);
          (*out_fn)(buf, len);
        } else {
          printf("Check failed: " "1" "==" "int2int_get(&h, 1, 0)" " at %s, %d\n", "test\\hashtable_test.cpp", 49);
        }
      } while (0);
      abort();
      exit(-1);
    }
  } while (0);
  int2int_del(&h, 1);
  do {
    if (!((0) == (int2int_get(&h, 1, 0)))) {
      do {
        if(0 != out_fn) {
          char buf[600];
          size_t len = _snprintf(buf, sizeof(buf), "Check failed: " "0" "==" "int2int_get(&h, 1, 0)" " at %s, %d\n", "test\\hashtable_test.cpp", 51);
          (*out_fn)(buf, len);
        } else {
          printf("Check failed: " "0" "==" "int2int_get(&h, 1, 0)" " at %s, %d\n", "test\\hashtable_test.cpp", 51);
        }
      } while (0);
      abort();
      exit(-1);
    }
  } while (0);

  int2int_destroy(&h);
}


void __cdecl test_hashtable_simple2_run(out_fn_t out_fn);
int test_hashtable_simple2_var = ADD_RUN_TEST("hashtable_simple2" , &test_hashtable_simple2_run);
void __cdecl test_hashtable_simple2_run(out_fn_t out_fn) {
  int_table_t h;
  int_table_init(&h, 3);
  int_table_put(&h, 1,1);
  do {
    if (!((1) == (int_table_get(&h, 1, 0)))) {
      do {
        if(0 != out_fn) {
          char buf[600];
          size_t len = _snprintf(buf, sizeof(buf), "Check failed: " "1" "==" "int_table_get(&h, 1, 0)" " at %s, %d\n", "test\\hashtable_test.cpp", 61);
          (*out_fn)(buf, len);
        } else {
          printf("Check failed: " "1" "==" "int_table_get(&h, 1, 0)" " at %s, %d\n", "test\\hashtable_test.cpp", 61);
        }
      } while (0);
      abort();
      exit(-1);
    }
  } while (0);
  int_table_del(&h, 1);
  do {
    if (!((0) == (int_table_get(&h, 1, 0)))) {
      do {
        if(0 != out_fn) {
          char buf[600];
          size_t len = _snprintf(buf, sizeof(buf), "Check failed: " "0" "==" "int_table_get(&h, 1, 0)" " at %s, %d\n", "test\\hashtable_test.cpp", 63);
          (*out_fn)(buf, len);
        } else {
          printf("Check failed: " "0" "==" "int_table_get(&h, 1, 0)" " at %s, %d\n", "test\\hashtable_test.cpp", 63);
        }
      } while (0);
      abort();
      exit(-1);
    }
  } while (0);

  int_table_destroy(&h);
}
