/*
 * Copyright (C) 2017  Ricardo Biehl Pasquali <rbpoficial@gmail.com>
 * under the terms of the GNU General Public License (see LICENSE file)
 *
 * A fixed-size memory allocator
 */

/*
 * How the allocator works?  Basically, we maintain a list of free chunks. Read
 * below to understand the details.
 *
 * A chunk is an allocation done with malloc(). The size of this allocation is
 * always the same for a given memory structure. Each chunk while free is used
 * as a pointer to the next free chunk (see `struct free_chunk` in allocator.h).
 * Also, notice that a free chunk is free to our allocator, but it's still an
 * unfreed memory to the kernel.
 *
 * A memory structure contains information about multiple allocations that
 * belong to a single list of free chunks. See `struct memory` in allocator.h
 *
 * Demonstration of the allocator behavior:
 * > Initial state -- all chunks are free.
 *     A -> B -> [NULL]
 * > When a chunk is allocated, it's always from head (the 1st) of the list.
 *     B -> [NULL]
 * > If list becomes empty and another chunk is requested, our behavior will
 *   depend of `auto_expand_factor` in `struct memory`. If `auto_expand_factor`
 *   is set to zero we return NULL, otherwise we increase list capacity by
 *   `auto_expand_factor` and return a valid chunk.
 * > When a chunk is freed it becomes the head of the list.
 */

#include <stdlib.h>

#include "allocator.h"


/*
 * note: size argument mustn't be zero!
 *
 * before expand:
 *   A -> B -> [NULL]
 * after expanded:
 *   C -> D -> A -> B -> [NULL]
 *
 *     * 1st element is the list head
 */
static void
expand_capacity(struct memory *mem, unsigned long size)
{
	struct free_chunk *new_head;
	struct free_chunk *current_chunk;
	unsigned long i;

	new_head = malloc(mem->chunk_size);
	current_chunk = new_head;

	/* if size is greater than 1 */
	for (i = 1; i < size; i++) {
		current_chunk->next = malloc(mem->chunk_size);
		current_chunk = current_chunk->next;
	}

	current_chunk->next = mem->free_list;
	mem->free_list = new_head;
	mem->capacity += size;
}

/*
 * in application code you can check whether free_list is reaching its end with:
 *   if (mem.capacity - mem.charge < X)
 *       allocator_expand_capacity(Y);
 */
int
allocator_expand_capacity(struct memory *mem, unsigned long size)
{
	if (! size)
		return -1;

	expand_capacity(mem, size);

	return 0;
}

void
allocator_free(struct memory *mem, void *ptr)
{
	struct free_chunk *free_chunk;

	free_chunk = ptr;

	/*
	 * insert the free chunk in the free_list -- we set the next of the new
	 * free chunk to the head of free_list, and set the head of free_list to
	 * the new free chunk
	 */
	free_chunk->next = mem->free_list;
	mem->free_list = free_chunk;

	mem->charge--;
}

void*
allocator_alloc(struct memory *mem, unsigned long size)
{
	struct free_chunk *free_chunk;

	if (size > mem->chunk_size)
		return NULL;

	/*
	 * expand capacity if there isn't any free chunk and auto_expand is
	 * enabled
	 *
	 * a.k.a.  if (mem->charge == mem->capacity)
	 */
	if (mem->free_list == NULL) {
		if (mem->auto_expand_factor)
			expand_capacity(mem, mem->auto_expand_factor);
		else
			return NULL;
	}

	/*
	 * remove the free chunk from the free_list -- we simply get the fist
	 * free chunk and set the head of free_list to the next free chunk
	 */
	free_chunk = mem->free_list;
	mem->free_list = free_chunk->next;

	mem->charge++;

	return free_chunk;
}

void
allocator_clean_up(struct memory *mem)
{
	struct free_chunk *current;

	while (mem->free_list) {
		current = mem->free_list;
		mem->free_list = current->next;
		free(current);
		mem->capacity--;
	}

	/*
	 * notice that here capacity is not necessarily zero, that's because
	 * there may be chunks currently in use
	 */
}

/*
 * check parameters, initialize memory struct, and allocate memory with
 * expand_capacity()
 */
int
allocator_init(struct memory *mem, unsigned long object_size,
	       unsigned long initial_size, unsigned long auto_expand_factor)
{
	if (!initial_size)
		return -1;

	/* chunk size must be at least the size of `struct free_chunk` */
	if (object_size < sizeof(struct free_chunk))
		mem->chunk_size = sizeof(struct free_chunk);
	else
		mem->chunk_size = object_size;

	mem->free_list = NULL;
	mem->auto_expand_factor = auto_expand_factor;
	mem->charge = 0;
	mem->capacity = 0;

	expand_capacity(mem, initial_size);

	return 0;
}
