/*
 * Copyright (C) 2017  Ricardo Biehl Pasquali <rbpoficial@gmail.com>
 * under the terms of the GNU General Public License (see LICENSE file)
 *
 * A fixed-size memory allocator
 */

/*
 * when a chunk is free it becomes a `struct free_chunk`, so we can maintain a
 * single linked list of free chunks (a free_list).
 * Therefore, if the chunk size you request is smaller than size of
 * `struct free_chunk` we set the chunk size as the size of `struct free_chunk`.
 */
struct free_chunk {
	struct free_chunk *next;
};

struct memory {
	/* head of the free_list */
	struct free_chunk *free_list;
	/* size of the chunks in this memory instance */
	unsigned long chunk_size;
	/*
	 * if set to a value different of 0 we automatically increase capacity
	 * by this value when free_list empties
	 */
	unsigned long auto_expand_factor;

	/* current charge level of allocator */
	unsigned long charge;
	/* current capacity of allocator */
	unsigned long capacity;
};


int
allocator_expand_capacity(struct memory *mem, unsigned long size);

void
allocator_free(struct memory *mem, void *ptr);

void*
allocator_alloc(struct memory *mem, unsigned long size);

void
allocator_clean_up(struct memory *mem);

int
allocator_init(struct memory *mem, unsigned long object_size,
	       unsigned long initial_size, int auto_expand_factor);
