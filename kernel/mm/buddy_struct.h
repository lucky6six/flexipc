#pragma once

#include <common/types.h>
#include <common/list.h>

#define BUDDY_PAGE_SHIFT    (12UL)
#define BUDDY_PAGE_SIZE     (1UL << BUDDY_PAGE_SHIFT)
/*
 * Supported Order: [0, BUDDY_MAX_ORDER).
 * The max allocated size (continous physical memory size) is
 * 2^(BUDDY_MAX_ORDER - 1) * 4K, i.e., 16M.
 */
#define BUDDY_MAX_ORDER     (14UL)

struct page {
	struct list_head list_node;
	u64 flags;
	u64 order;
	void *slab;
};

struct free_list {
	struct list_head list_head;
	u64 nr_free;
};

struct global_mem {
	unsigned long page_num;
	unsigned long page_size;
	/* first_page: the start vaddr of the metadata area */
	struct page *first_page;
	/* start_addr, end_addr: the range (vaddr) of physical memory */
	unsigned long start_addr;
	unsigned long end_addr;
	struct free_list free_lists[BUDDY_MAX_ORDER];
};
