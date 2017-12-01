/*
 * main.c
 *
 *  Created on: 29 lis 2017
 *      Author: Mati
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "memp.h"
#include "mempool.h"

/*
 * __wrap_malloc - malloc wrapper function
 */
void *__wrap_malloc(size_t size)
{
	void *ptr = NULL;

    ptr = mempool_malloc(size);

    printf("malloc(%d) = %p\n", size, ptr);
    return ptr;
}

/*
 * __wrap_free - free wrapper function
 */
void __wrap_free(void *ptr)
{

	mempool_free(ptr);
    printf("free(%p)\n", ptr);
}

int main(void)
{
	mempool_stats_display();
	fflush(stdout);
	printf("hello\n");

	return 0;
}


