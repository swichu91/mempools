/*
 * mem.c
 *
 *  Created on: Nov 29, 2017
 *      Author: mati
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "memp.h"


#define MEM_USE_POOLS_TRY_BIGGER_POOL 1

/**
 * Allocate memory: determine the smallest pool that is big enough
 * to contain an element of 'size' and get an element from that pool.
 *
 * @param size the size in bytes of the memory needed
 * @return a pointer to the allocated memory or NULL if the pool is empty
 */
void * mem_malloc(size_t size)
{
  void *ret;
  struct memp_malloc_helper *element = NULL;
  memp_t poolnr;
  size_t required_size = size + MEMP_ALIGN_SIZE(sizeof(struct memp_malloc_helper));

  for (poolnr = MEMP_POOL_FIRST; poolnr <= MEMP_POOL_LAST; poolnr = (memp_t)(poolnr + 1)) {
    /* is this pool big enough to hold an element of the required size
       plus a struct memp_malloc_helper that saves the pool this element came from? */
    if (required_size <= memp_pools[poolnr]->size) {
      element = (struct memp_malloc_helper*)memp_malloc(poolnr);
      if (element == NULL) {
        /* No need to DEBUGF or ASSERT: This error is already taken care of in memp.c */
#if MEM_USE_POOLS_TRY_BIGGER_POOL
        /** Try a bigger pool if this one is empty! */
        if (poolnr < MEMP_POOL_LAST) {
          continue;
        }
#endif /* MEM_USE_POOLS_TRY_BIGGER_POOL */
        printf("mem_malloc(): No free memory!\n");
        return NULL;
      }
      break;
    }
  }
  if (poolnr > MEMP_POOL_LAST) {
	  printf("mem_malloc(): no pool is that big!\n");
    return NULL;
  }

  /* save the pool number this element came from */
  element->poolnr = poolnr;
  /* and return a pointer to the memory directly after the struct memp_malloc_helper */
  ret = (uint8_t*)element + MEMP_ALIGN_SIZE(sizeof(struct memp_malloc_helper));

#if MEMP_OVERFLOW_CHECK || MEM_STATS
  /* truncating to u16_t is safe because struct memp_desc::size is u16_t */
  element->size = (uint16_t)size;
 // MEM_STATS_INC_USED(used, element->size);
#endif /* MEMP_OVERFLOW_CHECK || MEM_STATS */
#if MEMP_OVERFLOW_CHECK
  /* initialize unused memory (diff between requested size and selected pool's size) */
  memset((uint8_t*)ret + size, 0xcd, memp_pools[poolnr]->size - size);
#endif /* MEMP_OVERFLOW_CHECK */
  return ret;
}

/**
 * Free memory previously allocated by mem_malloc. Loads the pool number
 * and calls memp_free with that pool number to put the element back into
 * its pool
 *
 * @param rmem the memory element to free
 */
void mem_free(void *rmem)
{
  struct memp_malloc_helper *hmem;

  /* get the original struct memp_malloc_helper */
  /* cast through void* to get rid of alignment warnings */
  hmem = (struct memp_malloc_helper*)(void*)((uint8_t*)rmem - MEMP_ALIGN_SIZE(sizeof(struct memp_malloc_helper)));

 // MEM_STATS_DEC_USED(used, hmem->size);
#if MEMP_OVERFLOW_CHECK
  {
     uint16_t i;
     //assert(hmem->size <= memp_pools[hmem->poolnr]->size,"MEM_USE_POOLS: invalid chunk size");
     /* check that unused memory remained untouched (diff between requested size and selected pool's size) */
     for (i = hmem->size; i < memp_pools[hmem->poolnr]->size; i++) {
        uint8_t data = *((uint8_t*)rmem + i);
       // assert(data == 0xcd,"MEM_USE_POOLS: mem overflow detected");
     }
  }
#endif /* MEMP_OVERFLOW_CHECK */

  /* and put it in the pool we saved earlier */
  memp_free(hmem->poolnr, hmem);
}
