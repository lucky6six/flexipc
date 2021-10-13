#include <common/types.h>
#include <common/util.h>
#include <common/macro.h>
#include <common/kprint.h>
#include <common/lock.h>

#include "buddy.h"

struct lock buddy_lock;

/*
 * local functions or types
 */

enum pageflags {
	PG_head,
	PG_buddy,
};

static inline void set_page_order_buddy(struct page *page, u64 order)
{
	page->order = order;
	page->flags |= (1UL << PG_buddy);
}

static inline void clear_page_order_buddy(struct page *page)
{
	page->order = 0;
	page->flags &= ~(1UL << PG_buddy);
}

static inline u64 get_order(struct page *page)
{
	return page->order;
}

static inline u64 get_page_idx(struct global_mem *zone, struct page *page)
{
	return (page - zone->first_page);
}

/*
 * return the start page (metadata) of the only one buddy.
 * e.g., page 0 and page 1 are buddies, page (0, 1) and page (2, 3) are buddies
 */
static inline struct page *get_buddy_page(struct global_mem *zone,
					  struct page *page,
					  u64 order)
{
	u64 page_idx;
	u64 buddy_idx;

	page_idx = get_page_idx(zone, page);
	/* find buddy index (to merge with) */
	buddy_idx = (page_idx ^ (1 << order));

	return zone->first_page + buddy_idx;
}

/* return the start page (metadata) after "merging" the page and its buddy */
static inline struct page *get_merge_page(struct global_mem *zone,
					  struct page *page,
					  u64 order)
{
	u64 page_idx;
	u64 merge_idx;

	page_idx  = get_page_idx(zone, page);
	merge_idx = page_idx & ~(1 << order);

	return zone->first_page + merge_idx;
}

static void split(struct global_mem *zone, struct page *page,
		   u64 low_order, u64 high_order,
		   struct free_list *list)
{
	u64 size;

	size = (1U << high_order);
	/* keep splitting */
	while (high_order > low_order) {
		list--;
		high_order--;
		size >>= 1;
		/* put into the corresponding free list */
		list_add(&page[size].list_node, &list->list_head);
		list->nr_free++;
		// set page order
		set_page_order_buddy(&page[size], high_order);
	}
}

static struct page *__alloc_page(struct global_mem *zone, u64 order)
{
	struct page *page;
	struct free_list *list;
	u64 current_order;

	/* search the free list one by one */
	for (current_order = order; current_order < BUDDY_MAX_ORDER;
	     current_order++) {
		list = zone->free_lists + current_order;
		if (list_empty(&list->list_head)) {
			continue;
		}

		/* find one and remove it from the free list */
		page = list_entry(list->list_head.next, struct page,
				  list_node);
		list_del(&page->list_node);
		list->nr_free--;

		clear_page_order_buddy(page);
		/* split and put remaining parts back to free lists */

		split(zone, page, order, current_order, list);

		page->order = order;
		page->flags |= (1UL << PG_head);

		return page;
	}
	return NULL;
}

/* check whether the buddy can be merged */
static inline bool check_buddy(struct page *page, u64 order)
{
	return (page->flags & (1UL << PG_buddy))
		&& (page->order == order);
}

/*
 * exported functions
 */

void init_buddy(struct global_mem *zone, struct page *first_page,
		vaddr_t first_page_vaddr, u64 page_num)
{
	u64 i;
	struct page *page;
	struct free_list *list;

	BUG_ON(lock_init(&buddy_lock) != 0);

	/* init global memory metadata */
	zone->page_num = page_num;
	zone->page_size = BUDDY_PAGE_SIZE;
	zone->first_page = first_page;
	zone->start_addr = first_page_vaddr;
	zone->end_addr = zone->start_addr + page_num * BUDDY_PAGE_SIZE;

	/* init each free list (different orders) */
	for (i = 0; i < BUDDY_MAX_ORDER; i++) {
	        list = zone->free_lists + i;
		init_list_head(&list->list_head);
		list->nr_free = 0;
	}

	/* zero the metadata area (struct page for each page) */
	memset((char*)first_page, 0, page_num * sizeof(struct page));

	/* init the metadata for each page */
	for (i = 0; i < page_num; i++) {
		/* Currently, 0~img_end is reserved, however, if we want to use them, we should reserve MP_BOOT_ADDR */
		page = zone->first_page + i;
		init_list_head(&page->list_node);
		buddy_free_pages(zone, page);
	}

	kdebug("mm: finish initing buddy memory system.\n");
}

struct page *buddy_get_pages(struct global_mem *zone, u64 order)
{
	struct page *page;

	if (order >= BUDDY_MAX_ORDER) {
		kinfo("order: %ld, BUDDY_MAX_ORDER: %ld\n",
		       order, BUDDY_MAX_ORDER);
		BUG_ON(1);
		return NULL;
	}

	/* XXX Perf: maybe a dedicated lock for each list? */
	lock(&buddy_lock);
	page = __alloc_page(zone, order);
	unlock(&buddy_lock);
	return page;
}

void buddy_free_pages(struct global_mem *zone, struct page *page)
{
	u64 order;
	struct page *buddy;

	order = get_order(page);
	lock(&buddy_lock);

	/* clear head flag */
	page->flags &= ~(1UL << PG_head);

	/*
	 * order range: [0, BUDDY_MAX_ORDER).
	 * merge can only happens when order is less than
	 * (BUDDY_MAX_ORDER - 1)
	 */
	while (order < BUDDY_MAX_ORDER - 1) {
		/* find the corresponding (only-one) buddy (pages) to merge */
		buddy = get_buddy_page(zone, page, order);

		/* check whether the buddy can be merged */
		if (!check_buddy(buddy, order))
			break;

		/* remove the buddy in its original free list */
		list_del(&buddy->list_node);
		zone->free_lists[order].nr_free--;
		/* clear the buddy's order and flag */
		clear_page_order_buddy(buddy);

		/* update page after merged */
		page = get_merge_page(zone, page, order);
		order++;
	}

	/* set the merge one's order and flag */
	set_page_order_buddy(page, order);
	list_add(&page->list_node, &zone->free_lists[order].list_head);
	zone->free_lists[order].nr_free++;

	unlock(&buddy_lock);
}

void *page_to_virt(struct global_mem *zone, struct page *page)
{
	u64 page_idx;
	u64 address;

	if (page == NULL)
		return NULL;

	page_idx = get_page_idx(zone, page);
	address = zone->start_addr + page_idx * BUDDY_PAGE_SIZE;

	return (void *)address;
}

struct page *virt_to_page(struct global_mem *zone, void *ptr)
{
	u64 page_idx;
	struct page *page;
	u64 address;

	address = (u64)ptr;

	if ((address < zone->start_addr) || (address > zone->end_addr)) {
		kinfo("[BUG] start_addr=0x%lx, end_addr=0x%lx, address=0x%lx\n",
		       zone->start_addr, zone->end_addr, address);
		BUG_ON(1);
		return NULL;
	}
	page_idx = (address - zone->start_addr) >> BUDDY_PAGE_SHIFT;

	page = zone->first_page + page_idx;
	return page;
}
