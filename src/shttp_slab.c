#include "internal.h"

#define SHTTP_SLAB_PAGE_MASK   3
#define SHTTP_SLAB_PAGE        0
#define SHTTP_SLAB_BIG         1
#define SHTTP_SLAB_EXACT       2
#define SHTTP_SLAB_SMALL       3

#if (SHTTP_PTR_SIZE == 4)

#define SHTTP_SLAB_PAGE_FREE   0
#define SHTTP_SLAB_PAGE_BUSY   0xffffffff
#define SHTTP_SLAB_PAGE_START  0x80000000

#define SHTTP_SLAB_SHIFT_MASK  0x0000000f
#define SHTTP_SLAB_MAP_MASK    0xffff0000
#define SHTTP_SLAB_MAP_SHIFT   16

#define SHTTP_SLAB_BUSY        0xffffffff

#else /* (SHTTP_PTR_SIZE == 8) */

#define SHTTP_SLAB_PAGE_FREE   0
#define SHTTP_SLAB_PAGE_BUSY   0xffffffffffffffff
#define SHTTP_SLAB_PAGE_START  0x8000000000000000

#define SHTTP_SLAB_SHIFT_MASK  0x000000000000000f
#define SHTTP_SLAB_MAP_MASK    0xffffffff00000000
#define SHTTP_SLAB_MAP_SHIFT   32

#define SHTTP_SLAB_BUSY        0xffffffffffffffff

#endif


#if (SHTTP_DEBUG_MALLOC)
#define shttp_slab_junk(p, size)     memset(p, 0xA5, size)
#else
#define shttp_slab_junk(p, size)
#endif

#define shttp_memzero(p, size)        memset(p, 0, size)

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t shttp_size_t;
typedef int    shttp_int_t;

static shttp_slab_page_t *shttp_slab_alloc_pages(shttp_slab_pool_t *pool,
    shttp_size_t pages);
static void shttp_slab_free_pages(shttp_slab_pool_t *pool, shttp_slab_page_t *page,
                                  shttp_size_t pages);



static int shttp_slab_max_size;
static int shttp_slab_exact_size;
static int shttp_slab_exact_shift;


void shttp_slab_init(shttp_slab_pool_t *pool) {
  u_char           *p;
  size_t            size;
  shttp_int_t         m;
  shttp_size_t        i, n, pages;
  shttp_slab_page_t  *slots;

  /*
   * slab_max_size ,即slab最大obj大小，默认为pagesize的一半
   * slab_exact_size, 一个临界值，这里暂不详细讨论，见下面的注释.
   */
  if (shttp_slab_max_size == 0) {
    shttp_slab_max_size = shttp_pagesize / 2;
    shttp_slab_exact_size = shttp_pagesize / (8 * sizeof(uintptr_t));
    for (n = shttp_slab_exact_size; n >>= 1; shttp_slab_exact_shift++) {
      /* void */
    }
  }

  /*
   * 最小的obj大小，nginx默认使用8字节
   * 即min_shift 为最小obj大小的幂指数 <=> 8 = pow(2, 3)
   */
  pool->min_size = 1 << pool->min_shift;

  p = (u_char *) pool + sizeof(shttp_slab_pool_t);
  size = pool->end - p;

  /*
   * 模拟脏数据，即每次alloc的内存都有脏数据
   */
  shttp_slab_junk(p, size);

  slots = (shttp_slab_page_t *) p;
  n = shttp_pagesize_shift - pool->min_shift;

  /*
   * slots是page管理节点的头节点，一个slots链表(如slots[n])里的page的obj大小相等
   * slots[n].slab 是一个uintptr_t类型，在32位系统，与unsigned等价，即有32bit可用
   * slab字段在不同的使用场景下代表不同含义，这也是nginx slab巧妙的地方之一.
   * 详细解释见下面注释.
   */
  for (i = 0; i < n; i++) {
    slots[i].slab = 0;
    slots[i].next = &slots[i];
    slots[i].prev = 0;
  }

  p += n * sizeof(shttp_slab_page_t);

  /*
   * 每个page分割成相等大小的obj, 一个page对应一个管理节点
   * pages，即内存池的大小可分配多少个page
   * 由于需要考虑内存对齐的问题，所以在init函数结尾会重新计算pages的大小
   * 所以，内存对齐会带来内存浪费(最大不超过pagesize*2)，但也会带来很多惊喜...
   */
  pages = (shttp_size_t) (size / (shttp_pagesize + sizeof(shttp_slab_page_t)));

  shttp_memzero(p, pages * sizeof(shttp_slab_page_t));

  pool->pages = (shttp_slab_page_t *) p;

  /*
   * 1.free链表，连接所有空闲的page的管理节点
   * 2.slots链表，连接所有在用的，而且有可用obj的page的管理节点
   * 3.已没有空闲obj的page的管理节点，处于“悬空”状态，既不在free，也不在slots里.
   *
   * 所以情况3的page在内存池处于"不可见"状态，给应用层分配内存的时候不用再去查询此类page
   * 只是在free，释放内存的时候，会被重新放到相应的slots链表，因为此时它又有可用节点了
   *
   * 不管是free链表还是slots链表，连接的都是page对应的管理节点，page是存放实际数据的,与链表没有半点关系.
   */
  pool->free.prev = 0;
  pool->free.next = (shttp_slab_page_t *) p;

  /*
   * pool->pages指向第一块page对应的管理节点
   * 此时slab字段表示的是连续的可用的page数量，初始化时候都为连续且可用，即为pages
   * 所以对于未使用的page，其对应的管理节点的slab表示以该page开始，连续可用的page数量
   * 在使用的过程中，反复的分配、回收，可用的page之间是通过链表连接，在物理内存里并不一定是连续的
   */
  pool->pages->slab = pages;
  pool->pages->next = &pool->free;
  pool->pages->prev = (uintptr_t) &pool->free;

  /*
   * start指向第一块page的首地址
   * 值得注意的是数据区首地址必须是pagesize对齐，后续很多操作都得益于内存对齐
   */
  pool->start = (uint8_t *)
                shttp_align_ptr((uintptr_t) p + pages * sizeof(shttp_slab_page_t),
                                shttp_pagesize);

  /*
   * 重新计算上一步内存对齐后，可用的page数量
   * 需要谨记的是page 与 page管理节点是一一对应的
   * 后续很多操作是根据page管理节点的下标来推算page页的实际地址.
   */
  m = pages - (pool->end - pool->start) / shttp_pagesize;
  if (m > 0) {
    pages -= m;
    pool->pages->slab = pages;
  }
}

void* shttp_slab_alloc(shttp_slab_pool_t *pool, size_t size) {
  size_t            s;
  uintptr_t         p, n, m, mask, *bitmap;
  shttp_size_t        i, slot, shift, map;
  shttp_slab_page_t  *page, *prev, *slots;

  /*
   * 如果需要分配的内存超过最大的obj大小，则以pagesize为单位进行整页分配
   */
  if (size >= shttp_slab_max_size) {

    TRACE("slab alloc: %zu", size);

    page = shttp_slab_alloc_pages(pool, (size >> shttp_pagesize_shift)
                                  + ((size % shttp_pagesize) ? 1 : 0));

    /*
     * 由于每个page与其管理节点是一一对应的，所以根据管理节点的偏移，很容易可计算出page的首地址
     * 1 << shttp_pagesize_shift 是一个page的大小
     */
    if (page) {
      p = (page - pool->pages) << shttp_pagesize_shift;
      p += (uintptr_t) pool->start;

    } else {
      p = 0;
    }

    goto done;
  }

  /*
   * 根据size，计算其对应哪个slot；
   * 假设最小obj为8字节，最大obj为2048字节，则slab分9个规模，分别为
   * 8 16 32 64 128 256 512 1024 2048
   */
  if (size > pool->min_size) {
    shift = 1;
    for (s = size - 1; s >>= 1; shift++) {
      /* void */
    }
    slot = shift - pool->min_shift;

  } else {
    size = pool->min_size;
    shift = pool->min_shift;
    slot = 0;
  }

  /*
   * 获取对应的page管理节点链表
   */
  slots = (shttp_slab_page_t *) ((u_char *) pool + sizeof(shttp_slab_pool_t));
  page = slots[slot].next;

  /*
   * slab存在空闲obj
   */
  if (page->next != page) {

    /*
     * slab规模分为三种:
     *  1. < shttp_slab_exact_shift
     *  2. = shttp_slab_exact_shift
     *  3. > shttp_slab_exact_shift
     *
     * 为什么要区分三类情况，这与shttp_slab_page_t.slab字段的使用直接相关;
     * 在32位操作系统，slab大小是4字节，即32bit
     * 对应以上三种情况，slab字段的使用分别是
     *  1.表示具体的obj大小，记录的是size_shift
     *  2.pagesize/slab_exact_size 刚好为32，所以slab可以当作bitmap使用，表示page里哪些obj可用
     *  3.高(16)位是bitmap，低(16)位是记录块大小;
     */
    if (shift < shttp_slab_exact_shift) {

      do {
        /*
         * < slab_exact_shift的情况，slab只用来记录obj的大小
         * 则page的bitmap就需要占用obj来存放
         */
        p = (page - pool->pages) << shttp_pagesize_shift;
        bitmap = (uintptr_t *) (pool->start + p);

        /*
         * (1 << (shttp_pagesize_shift - shift)) 算出一个page存放的obj数目
         *  / (sizeof(uintptr_t) * 8) 计算需要占用多少uintptr_t 来存储bitmap
         *
         *  即map 表示占用多少个uintptr_t（32bit系统占4字节，64bit占8字节）
         */
        map = (1 << (shttp_pagesize_shift - shift))
              / (sizeof(uintptr_t) * 8);

        for (n = 0; n < map; n++) {

          /*
           * 如果有空闲obj
           */
          if (bitmap[n] != SHTTP_SLAB_BUSY) {

            /*
             * 找出具体的空闲obj
             * bit为0为空闲节点
             */
            for (m = 1, i = 0; m; m <<= 1, i++) {
              if ((bitmap[n] & m)) {
                continue;
              }

              /*
               * 更新bitmap
               */
              bitmap[n] |= m;

              /*
               *  计算得出i为该空闲obj在page里的地址偏移
               *  可拆分为 (1<<shift) * ((n*sizeof(uintptr_t)*8) + i) 理解
               *  ((n*sizeof(uintptr_t)*8) + i) 表示第n个obj
               *  (1<<shift) 为一个obj的大小
               */
              i = ((n * sizeof(uintptr_t) * 8) << shift)
                  + (i << shift);

              /*
               * 接下来的逻辑是遍历整个bitmap
               * 如果该page没有空闲obj，则把该page的管理节点从链表里删除，即该page处于“悬空”状态
               * 所以，再一次确认,slots链表里连接的是有可用obj的page的管理节点
               */
              if (bitmap[n] == SHTTP_SLAB_BUSY) {
                for (n = n + 1; n < map; n++) {
                  if (bitmap[n] != SHTTP_SLAB_BUSY) {
                    p = (uintptr_t) bitmap + i;

                    goto done;
                  }
                }

                /*
                 * (page->prev & ~SHTTP_SLAB_PAGE_MASK) 获取原始的prev的page的地址
                 * nginx代码经常看到内存对齐,这么做会带来性能的提升，带同时会造成小部分内存浪费
                 * 所以nginx很多时候会把某小部分信息，隐藏在地址里，通过简单的“或”，“与”运算来设置和还原.
                 *
                 * prev隐藏的信息是page对应的规模类型(small.exact,big.page).后面会详细讨论。
                 */
                prev = (shttp_slab_page_t *)
                       (page->prev & ~SHTTP_SLAB_PAGE_MASK);
                prev->next = page->next;
                page->next->prev = page->prev;

                /*
                 * SHTTP_SLAB_SMALL 表示 < shttp_slab_exact_shift 的slab类型
                 */
                page->next = NULL;
                page->prev = SHTTP_SLAB_SMALL;
              }

              /*
               * 正如上述注释，i表示obj在该page(page)里的偏移
               */
              p = (uintptr_t) bitmap + i;

              goto done;
            }
          }
        }

        page = page->next;

      } while (page);

      /*
       * slab_exact 类型；假设pagesize为4096,
       * 则slab_exact_size 为128
       */
    } else if (shift == shttp_slab_exact_shift) {

      do {
        if (page->slab != SHTTP_SLAB_BUSY) {

          /*
           * slab字段用作bitmap
           * for循环是为了找到空闲的obj
           */
          for (m = 1, i = 0; m; m <<= 1, i++) {
            if ((page->slab & m)) {
              continue;
            }

            page->slab |= m;

            if (page->slab == SHTTP_SLAB_BUSY) {
              prev = (shttp_slab_page_t *)
                     (page->prev & ~SHTTP_SLAB_PAGE_MASK);
              prev->next = page->next;
              page->next->prev = page->prev;

              page->next = NULL;
              page->prev = SHTTP_SLAB_EXACT;
            }

            /*
             * (page - pool->pages) << shttp_pagesize_shift 算出该page的首地址
             * i << shift 算出obj在page里的偏移
             * += pool->start 算出obj的首地址
             */
            p = (page - pool->pages) << shttp_pagesize_shift;
            p += i << shift;
            p += (uintptr_t) pool->start;

            goto done;
          }
        }

        page = page->next;

      } while (page);

    } else {
      /*
       *  > shttp_slab_exact_shift
       *  (page->slab & SHTTP_SLAB_SHIFT_MASK) => 取page对应的obj的size_shift
       *  1 << n 算出page里存储的obj数
       *  ((uintptr_t) 1 << n) - 1 => 算出bitmap 掩码
       *  n << SHTTP_SLAB_MAP_SHIFT 因为是高位保存bitmap数据，所以需要掩码往高位移
       */
      n = shttp_pagesize_shift - (page->slab & SHTTP_SLAB_SHIFT_MASK);
      n = 1 << n;
      n = ((uintptr_t) 1 << n) - 1;
      mask = n << SHTTP_SLAB_MAP_SHIFT;

      /*
       * 接下来的操作与之前类似
       */
      do {
        if ((page->slab & SHTTP_SLAB_MAP_MASK) != mask) {

          for (m = (uintptr_t) 1 << SHTTP_SLAB_MAP_SHIFT, i = 0;
               m & mask;
               m <<= 1, i++) {
            if ((page->slab & m)) {
              continue;
            }

            page->slab |= m;

            if ((page->slab & SHTTP_SLAB_MAP_MASK) == mask) {
              prev = (shttp_slab_page_t *)
                     (page->prev & ~SHTTP_SLAB_PAGE_MASK);
              prev->next = page->next;
              page->next->prev = page->prev;

              page->next = NULL;
              page->prev = SHTTP_SLAB_BIG;
            }

            p = (page - pool->pages) << shttp_pagesize_shift;
            p += i << shift;
            p += (uintptr_t) pool->start;

            goto done;
          }
        }

        page = page->next;

      } while (page);
    }
  }

  /*
   * 如果slots链表为空，即没有可用的pagee
   * 则重新分配一个page,并把其管理节点放到slots链表
   */
  page = shttp_slab_alloc_pages(pool, 1);

  if (page) {
    if (shift < shttp_slab_exact_shift) {
      p = (page - pool->pages) << shttp_pagesize_shift;
      bitmap = (uintptr_t *) (pool->start + p);

      /*
       * n为需要多少个obj用来存放bitmap
       */
      s = 1 << shift;
      n = (1 << (shttp_pagesize_shift - shift)) / 8 / s;

      if (n == 0) {
        n = 1;
      }

      /*
       * 正如上述所说，n代表占用多少个obj来存放bitmap
       * 所以，bitmap[0]初始化需要把占用的obj对应的bit置为1
       */
      bitmap[0] = (2 << n) - 1;

      /*
       * map为bitmap所覆盖的uintptr_t数
       */
      map = (1 << (shttp_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

      for (i = 1; i < map; i++) {
        bitmap[i] = 0;
      }

      /*
       * 把新分配的page对应的管理节点放到slots链表
       * (uintptr_t) &slots[slot] | SHTTP_SLAB_SMALL，在prev字段里保存了slab的规模(small,exact,big,page四类)
       * 这样做的好处主要是简化了free的逻辑;在free函数再详细讨论
       */
      page->slab = shift;
      page->next = &slots[slot];
      page->prev = (uintptr_t) &slots[slot] | SHTTP_SLAB_SMALL;

      slots[slot].next = page;

      p = ((page - pool->pages) << shttp_pagesize_shift) + s * n;
      p += (uintptr_t) pool->start;

      goto done;

    } else if (shift == shttp_slab_exact_shift) {

      page->slab = 1;
      page->next = &slots[slot];
      page->prev = (uintptr_t) &slots[slot] | SHTTP_SLAB_EXACT;

      slots[slot].next = page;

      p = (page - pool->pages) << shttp_pagesize_shift;
      p += (uintptr_t) pool->start;

      goto done;

    } else { /* shift > shttp_slab_exact_shift */

      page->slab = ((uintptr_t) 1 << SHTTP_SLAB_MAP_SHIFT) | shift;
      page->next = &slots[slot];
      page->prev = (uintptr_t) &slots[slot] | SHTTP_SLAB_BIG;

      slots[slot].next = page;

      p = (page - pool->pages) << shttp_pagesize_shift;
      p += (uintptr_t) pool->start;

      goto done;
    }
  }

  p = 0;

done:

  TRACE("slab alloc: %p", (void *)p);

  return (void *) p;
}

void shttp_slab_free(shttp_slab_pool_t *pool, void *p) {
  size_t            size;
  uintptr_t         slab, m, *bitmap;
  shttp_size_t        n, type, slot, shift, map;
  shttp_slab_page_t  *slots, *page;

  TRACE("slab free: %p", p);

  if ((u_char *) p < pool->start || (u_char *) p > pool->end) {
    ERR("shttp_slab_free(): outside of pool");
    goto fail;
  }

  /*
   * 算出p所在的是第n块page
   * type 为page里obj的规模:
   *  1. SMALL 即 < slab_exact_size
   *  2. EXACT 即 = slab_exact_size
   *  3. BIG   即 > slab_exact_size && < max_slab_size
   *  4. PAGE  即 > max_slab_size
   *
   *  不同的规模，free逻辑会不一样，看下面的注释
   */
  n = ((u_char *) p - pool->start) >> shttp_pagesize_shift;
  page = &pool->pages[n];
  slab = page->slab;
  type = page->prev & SHTTP_SLAB_PAGE_MASK;

  switch (type) {

  case SHTTP_SLAB_SMALL:

    /*
     * slab保存的是obj的大小的shift
     */
    shift = slab & SHTTP_SLAB_SHIFT_MASK;
    size = 1 << shift;

    /*
     * 得益于内存对齐，可做合法性弱校验
     */
    if ((uintptr_t) p & (size - 1)) {
      goto wrong_chunk;
    }

    /*
     * 1.算出p对应page里的第n个obj
     * 2.算出obj所对应某块(具体哪块是步骤3算出)bitmap的第m个bit
     * 3.算出obj所在的是哪块bitmap(一个uintptr_t为一块bitmap)
     * 4.算出bitmap的首地址(即第一块bitmap的地址)
     */
    n = ((uintptr_t) p & (shttp_pagesize - 1)) >> shift;
    m = (uintptr_t) 1 << (n & (sizeof(uintptr_t) * 8 - 1));
    n /= (sizeof(uintptr_t) * 8);
    bitmap = (uintptr_t *) ((uintptr_t) p & ~(shttp_pagesize - 1));

    /*
     * 检测是否对应的bit被置位了，如果否，则直接返回已释放
     * 避免重复释放带来副作用
     */
    if (bitmap[n] & m) {

      /*
       * 如果是free以后使得page由busy=>可用，
       * 则把该page重新放到slots链表管理
       */
      if (page->next == NULL) {
        slots = (shttp_slab_page_t *)
                ((u_char *) pool + sizeof(shttp_slab_pool_t));
        slot = shift - pool->min_shift;

        page->next = slots[slot].next;
        slots[slot].next = page;

        page->prev = (uintptr_t) &slots[slot] | SHTTP_SLAB_SMALL;
        page->next->prev = (uintptr_t) page | SHTTP_SLAB_SMALL;
      }

      bitmap[n] &= ~m;

      /*
       * 算出bitmap占用n个obj
       */
      n = (1 << (shttp_pagesize_shift - shift)) / 8 / (1 << shift);

      if (n == 0) {
        n = 1;
      }

      /*
       * 检查该page是否完全空闲，即是否还有已用obj
       * 如果没有，则把page重新放回free链表
       */
      if (bitmap[0] & ~(((uintptr_t) 1 << n) - 1)) {
        goto done;
      }

      map = (1 << (shttp_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

      for (n = 1; n < map; n++) {
        if (bitmap[n]) {
          goto done;
        }
      }

      shttp_slab_free_pages(pool, page, 1);

      goto done;
    }

    goto chunk_already_free;

  case SHTTP_SLAB_EXACT:

    /*
     * p对应bitmap的第m个bit
     */
    m = (uintptr_t) 1 <<
        (((uintptr_t) p & (shttp_pagesize - 1)) >> shttp_slab_exact_shift);
    size = shttp_slab_exact_size;

    if ((uintptr_t) p & (size - 1)) {
      goto wrong_chunk;
    }

    if (slab & m) {
      if (slab == SHTTP_SLAB_BUSY) {
        slots = (shttp_slab_page_t *)
                ((u_char *) pool + sizeof(shttp_slab_pool_t));
        slot = shttp_slab_exact_shift - pool->min_shift;

        page->next = slots[slot].next;
        slots[slot].next = page;

        page->prev = (uintptr_t) &slots[slot] | SHTTP_SLAB_EXACT;
        page->next->prev = (uintptr_t) page | SHTTP_SLAB_EXACT;
      }

      page->slab &= ~m;

      if (page->slab) {
        goto done;
      }

      shttp_slab_free_pages(pool, page, 1);

      goto done;
    }

    goto chunk_already_free;

  case SHTTP_SLAB_BIG:

    shift = slab & SHTTP_SLAB_SHIFT_MASK;
    size = 1 << shift;

    if ((uintptr_t) p & (size - 1)) {
      goto wrong_chunk;
    }

    /*
     * (((uintptr_t) p & (shttp_pagesize - 1)) >> shift) 算出对应bitmap哪个bit
     */
    m = (uintptr_t) 1 << ((((uintptr_t) p & (shttp_pagesize - 1)) >> shift)
                          + SHTTP_SLAB_MAP_SHIFT);

    if (slab & m) {

      if (page->next == NULL) {
        slots = (shttp_slab_page_t *)
                ((u_char *) pool + sizeof(shttp_slab_pool_t));
        slot = shift - pool->min_shift;

        page->next = slots[slot].next;
        slots[slot].next = page;

        page->prev = (uintptr_t) &slots[slot] | SHTTP_SLAB_BIG;
        page->next->prev = (uintptr_t) page | SHTTP_SLAB_BIG;
      }

      page->slab &= ~m;

      if (page->slab & SHTTP_SLAB_MAP_MASK) {
        goto done;
      }

      shttp_slab_free_pages(pool, page, 1);

      goto done;
    }

    goto chunk_already_free;

  case SHTTP_SLAB_PAGE:

    if ((uintptr_t) p & (shttp_pagesize - 1)) {
      goto wrong_chunk;
    }

    if (slab == SHTTP_SLAB_PAGE_FREE) {
      alert("shttp_slab_free(): page is already free");
      goto fail;
    }

    if (slab == SHTTP_SLAB_PAGE_BUSY) {
      alert("shttp_slab_free(): pointer to wrong page");
      goto fail;
    }

    n = ((u_char *) p - pool->start) >> shttp_pagesize_shift;
    size = slab & ~SHTTP_SLAB_PAGE_START;

    shttp_slab_free_pages(pool, &pool->pages[n], size);

    shttp_slab_junk(p, size << shttp_pagesize_shift);

    return;
  }

  /* not reached */

  return;

done:

  shttp_slab_junk(p, size);

  return;

wrong_chunk:

  ERR("shttp_slab_free(): pointer to wrong chunk");

  goto fail;

chunk_already_free:

  ERR("shttp_slab_free(): chunk is already free");

fail:

  return;
}

static shttp_slab_page_t * shttp_slab_alloc_pages(shttp_slab_pool_t *pool, shttp_size_t pages) {
  shttp_slab_page_t  *page, *p;

  for (page = pool->free.next; page != &pool->free; page = page->next) {

    /*
     * 上面提到过，对于空闲的page，其对应的管理节点的slab字段表示以该page开始，连续可用的page数
     * 值得注意的是，如果可用的空闲内存(page)总和超过size，但是由于不是连续的，也会导致分配失败
     * nginx定义上的连续并不严谨，即有可能把实际上连续内存当作非连续看待
     * 对于上面问题的理解，建议结合本人博客 http://www.dcshi.com/?p=360 m的图例理解
     */
    if (page->slab >= pages) {

      /*
       * 如果连续的page数比pages要大，进行分割，把剩余的page放回free链表里
       */
      if (page->slab > pages) {
        /*
         * 连续的page数要 减去 pages
         */
        page[pages].slab = page->slab - pages;
        page[pages].next = page->next;
        page[pages].prev = page->prev;

        p = (shttp_slab_page_t *) page->prev;
        p->next = &page[pages];
        page->next->prev = (uintptr_t) &page[pages];

      } else {
        p = (shttp_slab_page_t *) page->prev;
        p->next = page->next;
        page->next->prev = page->prev;
      }

      /*
       * slab使用:
       * 对于整page分配的情况，slab 记录两个信息
       *  1.标识是整page分配,即 SHTTP_SLAB_PAGE_START
       *  2.标识本次分配的page数量, 即pages
       *
       * next,prev 两个指针都处于悬空状态，会导致出现"野指针"的问题么？
       * 肯定是不会的，只需要free的时候把其放回free链表即可.
       */
      page->slab = pages | SHTTP_SLAB_PAGE_START;
      page->next = NULL;
      page->prev = SHTTP_SLAB_PAGE;

      if (--pages == 0) {
        return page;
      }

      /*
       * 一次分配超过一个page，则需要把第一块以外page对应的管理结构也进行更新
       */
      for (p = page + 1; pages; pages--) {
        p->slab = SHTTP_SLAB_PAGE_BUSY;
        p->next = NULL;
        p->prev = SHTTP_SLAB_PAGE;
        p++;
      }

      return page;
    }
  }

  ERR("shttp_slab_alloc() failed: no memory");

  return NULL;
}

static void shttp_slab_free_pages(shttp_slab_pool_t *pool, shttp_slab_page_t *page, shttp_size_t pages) {
  shttp_slab_page_t  *prev;

  page->slab = pages--;

  if (pages) {
    shttp_memzero(&page[1], pages * sizeof(shttp_slab_page_t));
  }

  if (page->next) {
    prev = (shttp_slab_page_t *) (page->prev & ~SHTTP_SLAB_PAGE_MASK);
    prev->next = page->next;
    page->next->prev = page->prev;
  }

  /*
   * 把回收的page重新放到free链表
   */
  page->prev = (uintptr_t) &pool->free;
  page->next = pool->free.next;

  page->next->prev = (uintptr_t) page;

  pool->free.next = page;
}

void shttp_slab_dummy_init(shttp_slab_pool_t *pool) {
  shttp_size_t n;

  /*
   * 内存池基于共享内存实现的场景
   * 外部进程attch同一块内存不需要重新初始化shttp_slab_pool_t
  */

  if (shttp_slab_max_size == 0) {
    shttp_slab_max_size = shttp_pagesize / 2;
    shttp_slab_exact_size = shttp_pagesize / (8 * sizeof(uintptr_t));
    for (n = shttp_slab_exact_size; n >>= 1; shttp_slab_exact_shift++) {
      /* void */
    }
  }
}

void shttp_slab_stat(shttp_slab_pool_t *pool, shttp_slab_stat_t *stat) {
  uintptr_t       m, n, mask, slab;
  uintptr_t       *bitmap;
  shttp_size_t      i, j, map, type, obj_size;
  shttp_slab_page_t   *page;

  shttp_memzero(stat, sizeof(shttp_slab_stat_t));

  page = pool->pages;
  stat->pages = (pool->end - pool->start) / shttp_pagesize;;

  for (i = 0; i < stat->pages; i++) {
    slab = page->slab;
    type = page->prev & SHTTP_SLAB_PAGE_MASK;

    switch (type) {

    case SHTTP_SLAB_SMALL:

      n = (page - pool->pages) << shttp_pagesize_shift;
      bitmap = (uintptr_t *) (pool->start + n);

      obj_size = 1 << slab;
      map = (1 << (shttp_pagesize_shift - slab))
            / (sizeof(uintptr_t) * 8);

      for (j = 0; j < map; j++) {
        for (m = 1 ; m; m <<= 1) {
          if ((bitmap[j] & m)) {
            stat->used_size += obj_size;
            stat->b_small   += obj_size;
          }

        }
      }

      stat->p_small++;

      break;

    case SHTTP_SLAB_EXACT:

      if (slab == SHTTP_SLAB_BUSY) {
        stat->used_size += sizeof(uintptr_t) * 8 * shttp_slab_exact_size;
        stat->b_exact   += sizeof(uintptr_t) * 8 * shttp_slab_exact_size;
      } else {
        for (m = 1; m; m <<= 1) {
          if (slab & m) {
            stat->used_size += shttp_slab_exact_size;
            stat->b_exact    += shttp_slab_exact_size;
          }
        }
      }

      stat->p_exact++;

      break;

    case SHTTP_SLAB_BIG:

      j = shttp_pagesize_shift - (slab & SHTTP_SLAB_SHIFT_MASK);
      j = 1 << j;
      j = ((uintptr_t) 1 << j) - 1;
      mask = j << SHTTP_SLAB_MAP_SHIFT;
      obj_size = 1 << (slab & SHTTP_SLAB_SHIFT_MASK);

      for (m = (uintptr_t) 1 << SHTTP_SLAB_MAP_SHIFT; m & mask; m <<= 1) {
        if ((page->slab & m)) {
          stat->used_size += obj_size;
          stat->b_big     += obj_size;
        }
      }

      stat->p_big++;

      break;

    case SHTTP_SLAB_PAGE:

      if (page->prev == SHTTP_SLAB_PAGE) {
        slab      =  slab & ~SHTTP_SLAB_PAGE_START;
        stat->used_size += slab * shttp_pagesize;
        stat->b_page    += slab * shttp_pagesize;
        stat->p_page    += slab;

        i += (slab - 1);

        break;
      }

    default:

      if (slab  > stat->max_free_pages) {
        stat->max_free_pages = page->slab;
      }

      stat->free_page += slab;

      i += (slab - 1);

      break;
    }

    page = pool->pages + i + 1;
  }

  stat->pool_size = pool->end - pool->start;
  stat->used_pct = stat->used_size * 100 / stat->pool_size;

  CRIT("pool_size : %zu bytes", stat->pool_size);
  CRIT("used_size : %zu bytes", stat->used_size);
  CRIT("used_pct  : %zu%%\n",   stat->used_pct);

  CRIT("total page count : %zu",  stat->pages);
  CRIT("free page count  : %zu\n",  stat->free_page);

  CRIT("small slab use page : %zu,\tbytes : %zu", stat->p_small, stat->b_small);
  CRIT("exact slab use page : %zu,\tbytes : %zu", stat->p_exact, stat->b_exact);
  CRIT("big   slab use page : %zu,\tbytes : %zu", stat->p_big,   stat->b_big);
  CRIT("page slab use page  : %zu,\tbytes : %zu\n", stat->p_page,  stat->b_page);

  CRIT("max free pages : %zu\n",    stat->max_free_pages);
}

#ifdef __cplusplus
};
#endif
