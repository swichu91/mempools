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
#include <stdbool.h>

#include "memp.h"

#define MEM_USE_POOLS_TRY_BIGGER_POOL 1

static bool is_initialized = false;

/**
 * Allocate memory: determine the smallest pool that is big enough
 * to contain an element of 'size' and get an element from that pool.
 *
 * @param size the size in bytes of the memory needed
 * @return a pointer to the allocated memory or NULL if the pool is empty
 */
void *
mempool_malloc (size_t size)
{

	if (!is_initialized)
	{
		memp_init ();
		is_initialized = true;
	}

	void *ret;
	struct memp_malloc_helper *element = NULL;
	memp_t poolnr;
	size_t required_size = size + MEMP_ALIGN_SIZE(sizeof(struct memp_malloc_helper));

	for (poolnr = MEMP_POOL_FIRST; poolnr <= MEMP_POOL_LAST; poolnr = (memp_t) (poolnr + 1))
	{
		/* is this pool big enough to hold an element of the required size
		 plus a struct memp_malloc_helper that saves the pool this element came from? */
		if (required_size <= memp_pools[poolnr]->size)
		{
			element = (struct memp_malloc_helper*) memp_malloc(poolnr);
			if (element == NULL)
			{
				/* No need to DEBUGF or ASSERT: This error is already taken care of in memp.c */
#if MEM_USE_POOLS_TRY_BIGGER_POOL
				/** Try a bigger pool if this one is empty! */
				if (poolnr < MEMP_POOL_LAST)
				{
					continue;
				}
#endif /* MEM_USE_POOLS_TRY_BIGGER_POOL */
#if MEMP_LOG
				printf("mem_malloc(): No free memory!\n");
#endif
				return NULL;
			}
			break;
		}
	}
	if (poolnr > MEMP_POOL_LAST)
	{
#if MEMP_LOG
		printf("mem_malloc(): no pool is that big!\n");
#endif
		return NULL;
	}

	/* save the pool number this element came from */
	element->poolnr = poolnr;
	/* and return a pointer to the memory directly after the struct memp_malloc_helper */
	ret = (uint8_t*) element + MEMP_ALIGN_SIZE(sizeof(struct memp_malloc_helper));

#if MEMP_OVERFLOW_CHECK || MEM_STATS
	element->size = size;
	// MEM_STATS_INC_USED(used, element->size);
#endif /* MEMP_OVERFLOW_CHECK || MEM_STATS */
#if MEMP_OVERFLOW_CHECK
	/* initialize unused memory (diff between requested size and selected pool's size) */
	memset ((uint8_t*) element + required_size, 0xcd, memp_pools[poolnr]->size - required_size);
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
void
mempool_free (void *rmem)
{
	struct memp_malloc_helper *hmem;

	/* get the original struct memp_malloc_helper */
	/* cast through void* to get rid of alignment warnings */
	hmem = (struct memp_malloc_helper*) (void*) ((uint8_t*) rmem - MEMP_ALIGN_SIZE(sizeof(struct memp_malloc_helper)));

#if MEMP_OVERFLOW_CHECK
	{
		uint16_t i;
		assert(hmem->size <= memp_pools[hmem->poolnr]->size && "MEM_USE_POOLS: invalid chunk size");
		/* check that unused memory remained untouched (diff between requested size and selected pool's size) */
		for (i = hmem->size + MEMP_ALIGN_SIZE(sizeof(struct memp_malloc_helper)); i < memp_pools[hmem->poolnr]->size;
				i++)
		{
			uint8_t data = *((uint8_t*) rmem + i);

			assert(data == 0xcd && "mem overflow detected");
		}
	}
#endif /* MEMP_OVERFLOW_CHECK */

	/* and put it in the pool we saved earlier */
	memp_free (hmem->poolnr, hmem);
}

/**
 * Display memory pools use statistic
 */
void
mempool_stats_display (void)
{

#if MEMP_STATS
	memp_t poolnr;
	for (poolnr = MEMP_POOL_FIRST; poolnr <= MEMP_POOL_LAST; poolnr = (memp_t) (poolnr + 1))
	{

		printf ("\nMEM %s\n\t", memp_pools[poolnr]->stats->name);
		printf ("avail: %lu \n\t", (uint32_t) memp_pools[poolnr]->stats->avail);
		printf ("used: %lu \n\t", (uint32_t) memp_pools[poolnr]->stats->used);
		printf ("max: %lu \n\t", (uint32_t) memp_pools[poolnr]->stats->max);
		printf ("err: %lu \n", (uint32_t) memp_pools[poolnr]->stats->err);
	}
#endif

}
