/*
 * memp.c
 *
 *  Created on: Nov 29, 2017
 *      Author: mati
 */
#include <stdint.h>
#include <stdio.h>
#include "memp.h"

/* Get the number of entries in an array ('x' must NOT be a pointer!) */
#define ARRAYSIZE(x) (sizeof(x)/sizeof((x)[0]))

#define MEMPOOL(name,num,size,desc) MEMPOOL_DECLARE(name,num,size,desc)
#include "pools.h"

const struct memp_desc* const memp_pools[MEMP_MAX] = {
#define MEMPOOL(name,num,size,desc) &memp_ ## name,
#include "pools.h"
};


static void* do_memp_malloc_pool(const struct memp_desc *desc);
static void do_memp_free_pool(const struct memp_desc* desc, void *mem);

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
void * memp_malloc(memp_t type)
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

#ifdef LWIP_HOOK_MEMP_AVAILABLE
  old_first = *memp_pools[type]->tab;
#endif

  do_memp_free_pool(memp_pools[type], mem);

#ifdef LWIP_HOOK_MEMP_AVAILABLE
  if (old_first == NULL) {
    LWIP_HOOK_MEMP_AVAILABLE(type);
  }
#endif
}




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

#if MEMP_SANITY_CHECK
  LWIP_ASSERT("memp sanity", memp_sanity(desc));
#endif /* MEMP_SANITY_CHECK */

}

static void* do_memp_malloc_pool(const struct memp_desc *desc)
{
  struct memp *memp;

  memp = *desc->tab;

  if (memp != NULL) {
#if !MEMP_MEM_MALLOC
#if MEMP_OVERFLOW_CHECK == 1
    memp_overflow_check_element_overflow(memp, desc);
    memp_overflow_check_element_underflow(memp, desc);
#endif /* MEMP_OVERFLOW_CHECK */

    *desc->tab = memp->next;
#if MEMP_OVERFLOW_CHECK
    memp->next = NULL;
#endif /* MEMP_OVERFLOW_CHECK */
#endif /* !MEMP_MEM_MALLOC */
#if MEMP_OVERFLOW_CHECK
    memp->file = file;
    memp->line = line;
#if MEMP_MEM_MALLOC
    memp_overflow_init_element(memp, desc);
#endif /* MEMP_MEM_MALLOC */
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
   //printf("memp_malloc: out of memory in pool %s\n", desc->desc);
#if MEMP_STATS
    desc->stats->err++;
#endif
  }
  return NULL;
}
