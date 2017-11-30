/*
 * main.c
 *
 *  Created on: 29 lis 2017
 *      Author: Mati
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "mem.h"
#include "memp.h"

/*
 * __wrap_malloc - malloc wrapper function
 */
void *__wrap_malloc(size_t size)
{
	void *ptr = NULL;

    ptr = mem_malloc(size);

    printf("malloc(%d) = %p\n", size, ptr);
    return ptr;
}

/*
 * __wrap_free - free wrapper function
 */
void __wrap_free(void *ptr)
{

	mem_free(ptr);
    printf("free(%p)\n", ptr);
}

int main(void)
{

	memp_init();

	void* ptr = mem_malloc(1024);


	//*(uint8_t*)((uint8_t*)ptr+1026) = 5;

	void* ptr1 = mem_malloc(1024);

	mem_free(ptr);


	printf("hello\n");

	return 0;
}


