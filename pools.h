/*
 * pools.h
 *
 *  Created on: Nov 29, 2017
 *      Author: mati
 */

#ifndef MALLOC_MEMPOOL
/* This treats "malloc pools" just like any other pool.
   The pools are a little bigger to provide 'size' as the amount of user data. */
#define MALLOC_MEMPOOL(num, size) MEMPOOL(POOL_##size, num, (size + MEMP_ALIGN_SIZE(sizeof(struct memp_malloc_helper))), "MALLOC_"#size)
#define MALLOC_MEMPOOL_START
#define MALLOC_MEMPOOL_END
#endif

MALLOC_MEMPOOL_START
MALLOC_MEMPOOL(1, 16)
/*MALLOC_MEMPOOL(10, 512)
MALLOC_MEMPOOL(2, 1024)*/
MALLOC_MEMPOOL_END


#undef MALLOC_MEMPOOL
#undef MALLOC_MEMPOOL_START
#undef MALLOC_MEMPOOL_END
#undef MEMPOOL
