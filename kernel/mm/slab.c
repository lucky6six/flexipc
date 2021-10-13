#include <common/macro.h>
#include <common/types.h>
#include <common/kprint.h>
#include <common/lock.h>
#include <common/debug.h>

#include "slab.h"
#include "buddy.h"

extern struct global_mem global_mem;

/* local variables */
slab_header_t *slabs[SLAB_MAX_ORDER + 1];
struct lock slabs_locks[SLAB_MAX_ORDER + 1];

/* local functions */
static inline u64 size_to_order(u64 size)
{
	u64 order = 0;
	int tmp = size;

	while (tmp > 1) {
		tmp >>= 1;
		order += 1;
	}
	if (size > (1 << order))
		order += 1;

	return order;
}

static inline u64 order_to_size(u64 order)
{
	return 1UL << order;
}

static void* alloc_slab_memory(u64 size)
{
	struct page *p_page, *page;
	void *addr;
	u64 order, page_num;
	void *page_addr;
	int i;

	order = size_to_order(size/BUDDY_PAGE_SIZE);
	p_page = buddy_get_pages(&global_mem, order);
	if (p_page == NULL) {
		kwarn("failed to alloc_slab_memory: out of memory\n");
		BUG_ON(1);
	}
	addr = page_to_virt(&global_mem, p_page);

	//BUG_ON(check_alignment((u64)addr, SLAB_INIT_SIZE));
	page_num = order_to_size(order);
	for (i = 0; i < page_num; i++) {
		page_addr = (void *)((u64)addr + i * BUDDY_PAGE_SIZE);
		page = virt_to_page(&global_mem, page_addr);
		page->slab = addr;
	}

	return addr;
}

static slab_header_t *init_slab_cache(int order, int size)
{
	void *addr;
	slab_slot_list_t *slot;
	slab_header_t *slab;
	u64 cnt, obj_size;
	int i;

	addr = alloc_slab_memory(size);
	slab = (slab_header_t *)addr;

	obj_size = order_to_size(order);
	/* the first slot is used as metadata */
	cnt = size / obj_size - 1;

	slot = (slab_slot_list_t *)(addr + obj_size);
	slab->free_list_head = (void *)slot;
	slab->next_slab = NULL;
	slab->order = order;

	/* the last slot has no next one */
	for (i = 0; i < cnt - 1; i++) {
		slot->next_free = (void *)((u64)slot + obj_size);
		slot = (slab_slot_list_t *)((u64)slot + obj_size);
	}
	slot->next_free = NULL;

	return slab;
}

static void* _alloc_in_slab_nolock(slab_header_t *slab_header, int order) {
	slab_slot_list_t* first_slot;
	void* next_slot;
	slab_header_t *next_slab;
	slab_header_t *new_slab;

	first_slot = (slab_slot_list_t*)(slab_header->free_list_head);
	if (likely(first_slot != NULL)) {
		next_slot = first_slot->next_free;
		slab_header->free_list_head = next_slot;
		return first_slot;
	}

	next_slab = slab_header->next_slab;
	while (next_slab != NULL) {
		first_slot = (slab_slot_list_t*)(next_slab->free_list_head);
		if (likely(first_slot != NULL)) {
			next_slot = first_slot->next_free;
			next_slab->free_list_head = next_slot;
			return first_slot;
		}
		next_slab = next_slab->next_slab;
	}

	/* Allocate a new slab */
	new_slab = init_slab_cache(order, SLAB_INIT_SIZE);
	new_slab->next_slab = slab_header;
	slabs[order] = new_slab;

	return _alloc_in_slab_nolock(new_slab, order);
}

static void* _alloc_in_slab(slab_header_t *slab_header, int order) {
	void* free_slot;

	lock(&slabs_locks[order]);
	free_slot = _alloc_in_slab_nolock(slab_header, order);
	unlock(&slabs_locks[order]);

	return free_slot;
}

/*
 * exported functions
 */

void init_slab()
{
	int order;

	/* slab obj size: 32, 64, 128, 256, 512, 1024, 2048 */
	for (order = SLAB_MIN_ORDER; order <= SLAB_MAX_ORDER; order++) {
		slabs[order] = init_slab_cache(order, SLAB_INIT_SIZE);
		lock_init(&slabs_locks[order]);
	}
	kdebug("mm: finish initing slab allocators\n");
}

void *alloc_in_slab(u64 size)
{
	int order;

	BUG_ON(size > order_to_size(SLAB_MAX_ORDER));

	order = (int)size_to_order(size);
	if (order < SLAB_MIN_ORDER)
		order = SLAB_MIN_ORDER;

	return _alloc_in_slab(slabs[order], order);
}

#if DETECTING_DOUBLE_FREE_IN_SLAB == ON
static int check_slot_is_free(slab_header_t *slab_header,
			      slab_slot_list_t* slot)
{
	slab_slot_list_t *cur_slot;
	slab_header_t *next_slab;

	cur_slot = (slab_slot_list_t*)(slab_header->free_list_head);

	while (cur_slot != NULL) {
		if (cur_slot == slot)
			return 1;
		cur_slot = (slab_slot_list_t *)cur_slot->next_free;
	}

	next_slab = slab_header->next_slab;
	while (next_slab != NULL) {

		cur_slot = (slab_slot_list_t*)(next_slab->free_list_head);

		while (cur_slot != NULL) {
			if (cur_slot == slot)
				return 1;
			cur_slot = (slab_slot_list_t *)cur_slot->next_free;
		}
		next_slab = next_slab->next_slab;
	}

	return 0;
}
#endif

void free_in_slab(void* addr)
{
	struct page *page;
	slab_header_t *slab;
	slab_slot_list_t *slot;
	int order;

	slot = (slab_slot_list_t *)addr;
	page = virt_to_page(&global_mem, addr);
	BUG_ON(page == NULL);

	slab = page->slab;
	order = slab->order;
	lock(&slabs_locks[order]);

	#if DETECTING_DOUBLE_FREE_IN_SLAB == ON
		/*
		 * SLAB double free detection: check whether the slot to free is
		 * already in the free list.
		 */
		if (check_slot_is_free(slab, slot) == 1) {
			kinfo("SLAB: double free detected. Address is %p\n",
			      (u64)slot);
			BUG_ON(1);
		}
	#endif

	slot->next_free = slab->free_list_head;
	slab->free_list_head = slot;
	unlock(&slabs_locks[order]);
}
