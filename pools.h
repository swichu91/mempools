/*
 * pools.h
 *
 *  Created on: Nov 29, 2017
 *      Author: mati
 */

/**
 * Define your custom memory pools set.
 * Please see example use info below:
 *
 * ============================================
 * Use:
 * 	MALLOC_MEMPOOL_START
	MALLOC_MEMPOOL("chunk count", "chunk size")
	.
	.
	.
	.
	MALLOC_MEMPOOL_END

	Important:
	Memory pools have to be defined between MALLOC_MEMPOOL_START and MALLOC_MEMPOOL_END tags.
	It is perfectly fine to define unlimited amount of memory pools with different chunk sizes and count.
	Please remember that it is not allowed to declare more than one MALLOC_MEMPOOL with the same "chunk size" e.g

 	MALLOC_MEMPOOL_START
	MALLOC_MEMPOOL(20, 512)
	MALLOC_MEMPOOL(24, 512)
	MALLOC_MEMPOOL_END

	is not allowed but:

	MALLOC_MEMPOOL(20, 128)
	MALLOC_MEMPOOL(20, 256)

	is allowed.


 */

#ifndef MALLOC_MEMPOOL
/* This treats "malloc pools" just like any other pool.
 The pools are a little bigger to provide 'size' as the amount of user data. */
#define MALLOC_MEMPOOL(num, size) MEMPOOL(POOL_##size, num, (size + MEMP_ALIGN_SIZE(sizeof(struct memp_malloc_helper))), "MALLOC_"#size)
#define MALLOC_MEMPOOL_START
#define MALLOC_MEMPOOL_END
#endif


MALLOC_MEMPOOL_START
MALLOC_MEMPOOL(10, 1024)
MALLOC_MEMPOOL(20, 512)
MALLOC_MEMPOOL_END

#undef MALLOC_MEMPOOL
#undef MALLOC_MEMPOOL_START
#undef MALLOC_MEMPOOL_END
#undef MEMPOOL
