#pragma once

#include "buddy_struct.h"

void init_buddy(struct global_mem *zone, struct page *start_page,
		vaddr_t start_addr, u64 page_num);

struct page *buddy_get_pages(struct global_mem *zone, u64 order);
void buddy_free_pages(struct global_mem *zone, struct page *page);

void *page_to_virt(struct global_mem *zone, struct page *page);
struct page *virt_to_page(struct global_mem *zone, void* ptr);
