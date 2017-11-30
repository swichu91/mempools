/*
 * memp.h
 *
 *  Created on: Nov 29, 2017
 *      Author: mati
 */

#ifndef MEMP_H_
#define MEMP_H_

#include <stdint.h>

/* run once with empty definition to handle all custom includes in pools.h */
#define MEMPOOL(name,num,size,desc)
#include "pools.h"

/** Create the list of all memory pools managed by memp. MEMP_MAX represents a NULL pool at the end */
typedef enum {
#define MEMPOOL(name,num,size,desc)  MEMP_##name,
#include "pools.h"
  MEMP_MAX
} memp_t;

extern const struct memp_desc* const memp_pools[MEMP_MAX];


/**
 * Set to memory alignment supported by your platform
 */
#define MEM_ALIGNMENT                   1
#define MEMP_OVERFLOW_CHECK 1
#define MEMP_LOG		0
#define MEMP_STATS	0

#ifndef MEM_ALIGN_BUFFER
#define MEM_ALIGN_BUFFER(size) (((size) + MEM_ALIGNMENT - 1U))
#endif


#define HELPER_MEM_ALIGN_SIZE(size) (((size) + MEM_ALIGNMENT - 1U) & ~(MEM_ALIGNMENT-1U))

#define MEM_ALIGN(addr) ((void *)(((uintptr_t)(addr) + MEM_ALIGNMENT - 1) & ~(uintptr_t)(MEM_ALIGNMENT-1)))

#define DECLARE_MEMORY_ALIGNED(variable_name, size) uint8_t variable_name[MEM_ALIGN_BUFFER(size)]

#if MEMP_OVERFLOW_CHECK
/* if MEMP_OVERFLOW_CHECK is turned on, we reserve some bytes at the beginning
 * and at the end of each element, initialize them as 0xcd and check
 * them later. */
/* If MEMP_OVERFLOW_CHECK is >= 2, on every call to memp_malloc or memp_free,
 * every single element in each pool is checked!
 * This is VERY SLOW but also very helpful. */
/* MEMP_SANITY_REGION_BEFORE and MEMP_SANITY_REGION_AFTER can be overridden in
 * lwipopts.h to change the amount reserved for checking. */

#define MEMP_SANITY_REGION_BEFORE  16
#if MEMP_SANITY_REGION_BEFORE > 0
#define MEMP_SANITY_REGION_BEFORE_ALIGNED    HELPER_MEM_ALIGN_SIZE(MEMP_SANITY_REGION_BEFORE)
#else
#define MEMP_SANITY_REGION_BEFORE_ALIGNED    0
#endif /* MEMP_SANITY_REGION_BEFORE*/

#define MEMP_SANITY_REGION_AFTER   16

#if MEMP_SANITY_REGION_AFTER > 0
#define MEMP_SANITY_REGION_AFTER_ALIGNED     HELPER_MEM_ALIGN_SIZE(MEMP_SANITY_REGION_AFTER)
#else
#define MEMP_SANITY_REGION_AFTER_ALIGNED     0
#endif /* MEMP_SANITY_REGION_AFTER*/

/* MEMP_SIZE: save space for struct memp and for sanity check */
#define MEMP_SIZE          (HELPER_MEM_ALIGN_SIZE(sizeof(struct memp)) + MEMP_SANITY_REGION_BEFORE_ALIGNED)
#define MEMP_ALIGN_SIZE(x) (HELPER_MEM_ALIGN_SIZE(x) + MEMP_SANITY_REGION_AFTER_ALIGNED)

#else /* MEMP_OVERFLOW_CHECK */

/* No sanity checks
 * We don't need to preserve the struct memp while not allocated, so we
 * can save a little space and set MEMP_SIZE to 0.
 */
#define MEMP_SIZE           0
#define MEMP_ALIGN_SIZE(x) (HELPER_MEM_ALIGN_SIZE(x))

#endif



#if MEMP_STATS
#define MEMPOOL_DECLARE_STATS_INSTANCE(name) static struct stats_mem name;
#define MEMPOOL_DECLARE_STATS_REFERENCE(name) &name,
#else
#define MEMPOOL_DECLARE_STATS_INSTANCE(name)
#define MEMPOOL_DECLARE_STATS_REFERENCE(name)
#endif

#if MEMP_LOG || MEMP_OVERFLOW_CHECK || MEMP_STATS
#define DECLARE_MEMPOOL_DESC(desc) (desc),
#else
#define DECLARE_MEMPOOL_DESC(desc)
#endif

/* Use a helper type to get the start and end of the user "memory pools" for mem_malloc */
typedef enum {
    /* Get the first (via:
       MEMP_POOL_HELPER_START = ((u8_t) 1*MEMP_POOL_A + 0*MEMP_POOL_B + 0*MEMP_POOL_C + 0)*/
    MEMP_POOL_HELPER_FIRST = ((uint8_t)
#define MEMPOOL(name,num,size,desc)
#define MALLOC_MEMPOOL_START 1
#define MALLOC_MEMPOOL(num, size) * MEMP_POOL_##size + 0
#define MALLOC_MEMPOOL_END
#include "pools.h"
    ) ,
    /* Get the last (via:
       MEMP_POOL_HELPER_END = ((u8_t) 0 + MEMP_POOL_A*0 + MEMP_POOL_B*0 + MEMP_POOL_C*1) */
    MEMP_POOL_HELPER_LAST = ((uint8_t)
#define MEMPOOL(name,num,size,desc)
#define MALLOC_MEMPOOL_START
#define MALLOC_MEMPOOL(num, size) 0 + MEMP_POOL_##size *
#define MALLOC_MEMPOOL_END 1
#include "pools.h"
    )
} memp_pool_helper_t;

#define MEMP_POOL_FIRST ((memp_t) MEMP_POOL_HELPER_FIRST)
#define MEMP_POOL_LAST   ((memp_t) MEMP_POOL_HELPER_LAST)


struct stats_mem {
#if MEMP_STATS
  const char *name;
#endif /* MEMP_STATS*/
  uint32_t err;
  uint32_t avail;
  uint32_t used;
  uint32_t max;
  uint32_t illegal;
};

struct memp {
  struct memp *next;
#if MEMP_OVERFLOW_CHECK
  const char *file;
  int line;
#endif /* MEMP_OVERFLOW_CHECK */
};

/** Memory pool descriptor */
struct memp_desc {
#if MEMP_OVERFLOW_CHECK || MEMP_LOG || MEMP_STATS
  /** Textual description */
  const char *desc;
#endif /* MEMP_OVERFLOW_CHECK || MEM_LOG */
#if MEMP_STATS
  /** Statistics */
  struct stats_mem *stats;
#endif

  /** Element size */
  uint16_t size;

#if !MEMP_MEM_MALLOC
  /** Number of elements */
  uint16_t num;

  /** Base address */
  uint8_t *base;

  /** First free element of each pool. Elements form a linked list. */
  struct memp **tab;
#endif /* MEMP_MEM_MALLOC */
};

/** This structure is used to save the pool one element came from.
 * This has to be defined here as it is required for pool size calculation. */
struct memp_malloc_helper
{
   memp_t poolnr;
#if MEMP_OVERFLOW_CHECK || MEM_STATS
   uint16_t size;
#endif /* MEMP_OVERFLOW_CHECK || MEM_STATS */
};


#define MEMPOOL_DECLARE(name,num,size,desc) \
  DECLARE_MEMORY_ALIGNED(memp_memory_ ## name ## _base, ((num) * (MEMP_SIZE + MEMP_ALIGN_SIZE(size)))); \
    \
  MEMPOOL_DECLARE_STATS_INSTANCE(memp_stats_ ## name) \
    \
  static struct memp *memp_tab_ ## name; \
    \
  const struct memp_desc memp_ ## name = { \
    DECLARE_MEMPOOL_DESC(desc) \
    MEMPOOL_DECLARE_STATS_REFERENCE(memp_stats_ ## name) \
	MEMP_ALIGN_SIZE(size), \
    (num), \
    memp_memory_ ## name ## _base, \
    &memp_tab_ ## name \
  };


void memp_init_pool(const struct memp_desc *desc);

void memp_init(void);

#if MEMP_OVERFLOW_CHECK
void *memp_malloc_fn(memp_t type, const char* file, const int line);
#define memp_malloc(t) memp_malloc_fn((t), __FILE__, __LINE__)
#else
void *memp_malloc(memp_t type);
#endif
void  memp_free(memp_t type, void *mem);



#endif /* MEMP_H_ */
