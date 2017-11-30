/*
 * memp.c
 *
 *  Created on: Nov 29, 2017
 *      Author: mati
 */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "memp.h"


/* Get the number of entries in an array ('x' must NOT be a pointer!) */
#define ARRAYSIZE(x) (sizeof(x)/sizeof((x)[0]))

#define MEMPOOL(name,num,size,desc) MEMPOOL_DECLARE(name,num,size,desc)
#include "pools.h"

const struct memp_desc* const memp_pools[MEMP_MAX] = {
#define MEMPOOL(name,num,size,desc) &memp_ ## name,
#include "pools.h"
};


#if MEMP_OVERFLOW_CHECK
/**
 * Check if a memp element was victim of an overflow
 * (e.g. the restricted area after it has been altered)
 *
 * @param p the memp element to check
 * @param desc the pool p comes from
 */
static void
memp_overflow_check_element_overflow(struct memp *p, const struct memp_desc *desc)
{
#if MEMP_SANITY_REGION_AFTER_ALIGNED > 0
  uint16_t k;
  uint8_t *m;
  m = (uint8_t*)p + MEMP_SIZE + desc->size;
  for (k = 0; k < MEMP_SANITY_REGION_AFTER_ALIGNED; k++) {
    if (m[k] != 0xcd) {
#if MEMP_LOG
      char errstr[128] = "detected memp overflow in pool ";
      printf(errstr,"%s %s",errstr,desc->desc);
#endif
      assert (0);

    }
  }
#else /* MEMP_SANITY_REGION_AFTER_ALIGNED > 0 */
#endif /* MEMP_SANITY_REGION_AFTER_ALIGNED > 0 */
}

/**
 * Check if a memp element was victim of an underflow
 * (e.g. the restricted area before it has been altered)
 *
 * @param p the memp element to check
 * @param desc the pool p comes from
 */
static void
memp_overflow_check_element_underflow(struct memp *p, const struct memp_desc *desc)
{
#if MEMP_SANITY_REGION_BEFORE_ALIGNED > 0
	uint16_t k;
  uint8_t *m;
  m = (uint8_t*)p + MEMP_SIZE - MEMP_SANITY_REGION_BEFORE_ALIGNED;
  for (k = 0; k < MEMP_SANITY_REGION_BEFORE_ALIGNED; k++) {
    if (m[k] != 0xcd) {
#if MEMP_LOG
      char errstr[128] = "detected memp underflow in pool ";
      printf(errstr,"%s %s",errstr,desc->desc);
#endif
      assert (0);
    }
  }
#else /* MEMP_SANITY_REGION_BEFORE_ALIGNED > 0 */
#endif /* MEMP_SANITY_REGION_BEFORE_ALIGNED > 0 */
}


/**
 * Initialize the restricted area of on memp element.
 */
static void
memp_overflow_init_element(struct memp *p, const struct memp_desc *desc)
{
#if MEMP_SANITY_REGION_BEFORE_ALIGNED > 0 || MEMP_SANITY_REGION_AFTER_ALIGNED > 0
  uint8_t *m;
#if MEMP_SANITY_REGION_BEFORE_ALIGNED > 0
  m = (uint8_t*)p + MEMP_SIZE - MEMP_SANITY_REGION_BEFORE_ALIGNED;
  memset(m, 0xcd, MEMP_SANITY_REGION_BEFORE_ALIGNED);
#endif
#if MEMP_SANITY_REGION_AFTER_ALIGNED > 0
  m = (uint8_t*)p + MEMP_SIZE + desc->size;
  memset(m, 0xcd, MEMP_SANITY_REGION_AFTER_ALIGNED);
#endif
#else /* MEMP_SANITY_REGION_BEFORE_ALIGNED > 0 || MEMP_SANITY_REGION_AFTER_ALIGNED > 0 */

#endif /* MEMP_SANITY_REGION_BEFORE_ALIGNED > 0 || MEMP_SANITY_REGION_AFTER_ALIGNED > 0 */
}

#if MEMP_OVERFLOW_CHECK >= 2
/**
 * Do an overflow check for all elements in every pool.
 *
 * @see memp_overflow_check_element for a description of the check
 */
static void
memp_overflow_check_all(void)
{
  u16_t i, j;
  struct memp *p;

  for (i = 0; i < MEMP_MAX; ++i) {
    p = (struct memp*)MEM_ALIGN(memp_pools[i]->base);
    for (j = 0; j < memp_pools[i]->num; ++j) {
      memp_overflow_check_element_overflow(p, memp_pools[i]);
      memp_overflow_check_element_underflow(p, memp_pools[i]);
      p = ALIGNMENT_CAST(struct memp*, ((uint8_t*)p + MEMP_SIZE + memp_pools[i]->size + MEMP_SANITY_REGION_AFTER_ALIGNED));
    }
  }
}
#endif /* MEMP_OVERFLOW_CHECK >= 2 */

#endif


static void do_memp_free_pool(const struct memp_desc* desc, void *mem)
{
  struct memp *memp;

  /* cast through void* to get rid of alignment warnings */
  memp = (struct memp *)(void *)((uint8_t*)mem - MEMP_SIZE);

#if MEMP_OVERFLOW_CHECK == 1
  memp_overflow_check_element_overflow(memp, desc);
  memp_overflow_check_element_underflow(memp, desc);
#endif /* MEMP_OVERFLOW_CHECK */

#if MEMP_STATS
  desc->stats->used--;
#endif

  memp->next = *desc->tab;
  *desc->tab = memp;

}

static void*
#if !MEMP_OVERFLOW_CHECK
do_memp_malloc_pool(const struct memp_desc *desc)
#else
do_memp_malloc_pool_fn(const struct memp_desc *desc, const char* file, const int line)
#endif
{
  struct memp *memp;

  memp = *desc->tab;

  if (memp != NULL) {
#if MEMP_OVERFLOW_CHECK == 1
    memp_overflow_check_element_overflow(memp, desc);
    memp_overflow_check_element_underflow(memp, desc);
#endif /* MEMP_OVERFLOW_CHECK */

    *desc->tab = memp->next;
#if MEMP_OVERFLOW_CHECK
    memp->next = NULL;
#endif /* MEMP_OVERFLOW_CHECK */

#if MEMP_OVERFLOW_CHECK
    memp->file = file;
    memp->line = line;
#endif /* MEMP_OVERFLOW_CHECK */

#if MEMP_STATS
    desc->stats->used++;
    if (desc->stats->used > desc->stats->max) {
      desc->stats->max = desc->stats->used;
    }
#endif
    /* cast through u8_t* to get rid of alignment warnings */
    return ((uint8_t*)memp + MEMP_SIZE);
  } else {
#if MEMP_LOG
   printf("memp_malloc: out of memory in pool %s\n", desc->desc);
#endif
#if MEMP_STATS
    desc->stats->err++;
#endif
  }
  return NULL;
}

void
memp_init_pool(const struct memp_desc *desc)
{
  int i;
  struct memp *memp;

  *desc->tab = NULL;
  memp = (struct memp*)MEM_ALIGN(desc->base);
  /* create a linked list of memp elements */
  for (i = 0; i < desc->num; ++i) {
    memp->next = *desc->tab;
    *desc->tab = memp;
#if MEMP_OVERFLOW_CHECK
    memp_overflow_init_element(memp, desc);
#endif /* MEMP_OVERFLOW_CHECK */
   /* cast through void* to get rid of alignment warnings */
   memp = (struct memp *)(void *)((uint8_t *)memp + MEMP_SIZE + desc->size
#if MEMP_OVERFLOW_CHECK
      + MEMP_SANITY_REGION_AFTER_ALIGNED
#endif
    );
  }
#if MEMP_STATS
  desc->stats->avail = desc->num;
#endif /* MEMP_STATS */

#if MEMP_STATS
  desc->stats->name  = desc->desc;
#endif /* MEMP_STATS*/
}

void memp_init(void)
{
  uint16_t i;

  /* for every pool: */
  for (i = 0; i < ARRAYSIZE(memp_pools); i++) {
    memp_init_pool(memp_pools[i]);

#if MEMP_STATS
    lwip_stats.memp[i] = memp_pools[i]->stats;
#endif
  }

#if MEMP_OVERFLOW_CHECK >= 2
  /* check everything a first time to see if it worked */
  memp_overflow_check_all();
#endif /* MEMP_OVERFLOW_CHECK >= 2 */
}

/**
 * Get an element from a specific pool.
 *
 * @param type the pool to get an element from
 *
 * @return a pointer to the allocated memory or a NULL pointer on error
 */
void *
#if !MEMP_OVERFLOW_CHECK
memp_malloc(memp_t type)
#else
memp_malloc_fn(memp_t type, const char* file, const int line)
#endif
{
  void *memp;

#if MEMP_OVERFLOW_CHECK >= 2
  memp_overflow_check_all();
#endif /* MEMP_OVERFLOW_CHECK >= 2 */

#if !MEMP_OVERFLOW_CHECK
  memp = do_memp_malloc_pool(memp_pools[type]);
#else
  memp = do_memp_malloc_pool_fn(memp_pools[type], file, line);
#endif

  return memp;
}

/**
 * Put an element back into its pool.
 *
 * @param type the pool where to put mem
 * @param mem the memp element to free
 */
void memp_free(memp_t type, void *mem)
{
  if (mem == NULL) {
    return;
  }

#if MEMP_OVERFLOW_CHECK >= 2
  memp_overflow_check_all();
#endif /* MEMP_OVERFLOW_CHECK >= 2 */

  do_memp_free_pool(memp_pools[type], mem);

}


