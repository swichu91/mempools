/*
 * mem.h
 *
 *  Created on: Nov 29, 2017
 *      Author: mati
 */


/**
 * Free memory previously allocated by mem_malloc. Loads the pool number
 * and calls memp_free with that pool number to put the element back into
 * its pool
 *
 * @param rmem the memory element to free
 */
void mempool_free(void *rmem);


/**
 * Allocate memory: determine the smallest pool that is big enough
 * to contain an element of 'size' and get an element from that pool.
 *
 * @param size the size in bytes of the memory needed
 * @return a pointer to the allocated memory or NULL if the pool is empty
 */
void * mempool_malloc(size_t size);


/**
 * Display memory stats from all allocated memory pools in
 */
void mempool_stats_display(void);
