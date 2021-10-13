#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <sys/mman.h>

/* unit test */
#include "minunit.h"
/* kernel/mm/xxx */
#include "buddy.h"
#include "slab.h"

#define ROUND 1000
#define NPAGES (128 * 1024)

#define START_VADDR phys_to_virt(24*1024*1024)

struct global_mem global_mem;

/* test buddy allocator */

static unsigned long buddy_num_free_page(struct global_mem *zone)
{
	unsigned long i, ret;

	ret = 0;
	for (i = 0; i < BUDDY_MAX_ORDER; ++i) {
		ret += zone->free_lists[i].nr_free;
	}
	return ret;
}

static void valid_page_idx(struct global_mem *zone, long idx)
{
	mu_assert((idx < zone->page_num) && (idx >= 0), "invalid page idx");
}

static unsigned long get_page_idx(struct global_mem *zone, struct page *page)
{
	long idx;

	idx = page - zone->first_page;
	valid_page_idx(zone, idx);

	return idx;
}

static void test_alloc(struct global_mem *zone, long n, long order)
{
	long i;
	struct page *page;

	for (i = 0; i < n; ++i) {
		page = buddy_get_pages(zone, order);
		mu_check(page != NULL);
		get_page_idx(zone, page);
	}
	return;
}

void test_buddy(void)
{
	void *start;
	unsigned long npages;
	unsigned long size;
	unsigned long start_addr;

	long nget;
	long ncheck;

	struct page *page;
	long i;

	/* free_mem_size: npages * 0x1000 */
	npages = NPAGES;

	/* PAGE_SIZE + page metadata size */
	size = npages * (0x1000 + sizeof(struct page));
	start = mmap((void *)-1, size, PROT_READ|PROT_WRITE,
		     MAP_SHARED|MAP_ANONYMOUS, -1, 0);

	/* skip the metadata area */
	start_addr = (unsigned long)(start + npages * sizeof(struct page));
	init_buddy(&global_mem, start, start_addr, npages);

	/* check the init state */
	nget = buddy_num_free_page(&global_mem);
	ncheck = npages / powl(2, BUDDY_MAX_ORDER - 1);
	//printf("ncheck is %ld\n", ncheck);
	mu_check(nget == ncheck);

	/* alloc single page for $npages times */
	test_alloc(&global_mem, npages, 0);

	/* should have 0 free pages */
	nget = buddy_num_free_page(&global_mem);
	ncheck = 0;
	mu_check(nget == ncheck);

	/* free all pages */
	for (i = 0; i < npages; ++i) {
		page = global_mem.first_page + i;
		buddy_free_pages(&global_mem, page);
	}
	nget = buddy_num_free_page(&global_mem);
	ncheck = npages / powl(2, BUDDY_MAX_ORDER - 1);
	mu_check(nget == ncheck);

	/* alloc 2-pages for $npages/2 times */
	test_alloc(&global_mem, npages/2, 1);

	/* should have 0 free pages */
	nget = buddy_num_free_page(&global_mem);
	ncheck = 0;
	mu_check(nget == ncheck);

	/* free all pages */
	for (i = 0; i < npages; i += 2) {
		page = global_mem.first_page + i;
		buddy_free_pages(&global_mem, page);
	}
	nget = buddy_num_free_page(&global_mem);
	ncheck = npages / powl(2, BUDDY_MAX_ORDER - 1);
	mu_check(nget == ncheck);

	/* alloc MAX_ORDER-pages for  */
	test_alloc(&global_mem, npages/powl(2, BUDDY_MAX_ORDER - 1),
		   BUDDY_MAX_ORDER - 1);

	/* should have 0 free pages */
	nget = buddy_num_free_page(&global_mem);
	ncheck = 0;
	mu_check(nget == ncheck);

	/* free all pages */
	for (i = 0; i < npages; i += powl(2, BUDDY_MAX_ORDER - 1)) {
		page = global_mem.first_page + i;
		buddy_free_pages(&global_mem, page);
	}
	nget = buddy_num_free_page(&global_mem);
	ncheck = npages / powl(2, BUDDY_MAX_ORDER - 1);
	mu_check(nget == ncheck);


	/* alloc single page for $npages times */
	test_alloc(&global_mem, npages, 0);

	/* should have 0 free pages */
	nget = buddy_num_free_page(&global_mem);
	ncheck = 0;
	mu_check(nget == ncheck);

	/* free a half pages */
	for (i = 0; i < npages; i += 2) {
		page = global_mem.first_page + i;
		buddy_free_pages(&global_mem, page);
	}
	nget = buddy_num_free_page(&global_mem);
	ncheck = npages / 2;
	mu_check(nget == ncheck);

	/* free another half pages */
	for (i = 1; i < npages; i += 2) {
		page = global_mem.first_page + i;
		buddy_free_pages(&global_mem, page);
	}
	nget = buddy_num_free_page(&global_mem);
	ncheck = npages / powl(2, BUDDY_MAX_ORDER - 1);
	mu_check(nget == ncheck);
}


/* test slab allocator */

extern slab_header_t *slabs[SLAB_MAX_ORDER + 1];

static void check_slab_range(slab_header_t *slab, void *addr, int cnt)
{
	if (!((addr > (void*)slab) && (addr < (void*)slab + SLAB_INIT_SIZE))) {
		printf("[cnt %d] slab addr: 0x%lx, slot addr: 0x%lx\n",
		       cnt, (long)slab, (long)addr);
		exit(-1);
	}
	mu_check((void *)addr > (void*)slab);
	mu_check((void *)addr < ((void*)slab + SLAB_INIT_SIZE));
}

static void dump_slab(int order, int cnt_check)
{
	slab_header_t *slab;
	slab_slot_list_t *slot;
	int cnt;

	slab = slabs[order];
	mu_check(slab != NULL);

	cnt = 0;
	while (slab != NULL) {
		slot = (slab_slot_list_t *)(slab->free_list_head);
		if (slot == NULL) break;

		do {
			cnt += 1;
			/* check addr range */
			check_slab_range(slab, (void *)slot, cnt);
			slot = slot->next_free;
		} while (slot != NULL);

		slab = slab->next_slab;
	}

	mu_check(cnt == cnt_check);
}

void test_slab(void)
{
	int i;
	int j;

	int cnt;
	int check_cnt;
	void **ptr;

	init_slab();

	/* check init state */
	for (i = SLAB_MIN_ORDER; i <= SLAB_MAX_ORDER; ++i) {
		printf("test: check slab %d\n", i);
		dump_slab(i, SLAB_INIT_SIZE / powl(2, i) - 1);
		printf("test: check slab %d done\n", i);
	}

	/* check slab alloc/free */
	ptr = malloc(SLAB_INIT_SIZE / powl(2, SLAB_MIN_ORDER) * sizeof(void*));
	mu_check(ptr != NULL);

	for (i = SLAB_MIN_ORDER; i <= SLAB_MAX_ORDER; ++i) {
		// printf("test: slab %d alloc/free\n", i);
		cnt = SLAB_INIT_SIZE / powl(2, i) - 1;
		for (j = 0; j < cnt; ++j) {
			ptr[j] = alloc_in_slab(powl(2, i));
			mu_check(ptr[j] != NULL);
			check_slab_range(slabs[i], ptr[j], j);
		}
		dump_slab(i, 0);

		for (j = 0; j < cnt; ++j) {
			free_in_slab(ptr[j]);
		}
		dump_slab(i, cnt);
	}

	// printf("test: trigger new slab allocation\n");
	/* trigger new slab allocations */
	for (i = SLAB_MIN_ORDER; i <= SLAB_MAX_ORDER; ++i) {
		/* needs two slabs */
		cnt = SLAB_INIT_SIZE / powl(2, i);
		for (j = 0; j < cnt; ++j) {
			ptr[j] = alloc_in_slab(powl(2, i));
			mu_check(ptr[j] != NULL);
			check_slab_range(slabs[i], ptr[j], j);
		}
		dump_slab(i, cnt - 2);

		for (j = 0; j < cnt; ++j) {
			free_in_slab(ptr[j]);
		}
		dump_slab(i, 2 * (cnt -1));
	}

	/* trigger a third slab allocation */
	free(ptr);

	for (i = SLAB_MIN_ORDER; i <= SLAB_MAX_ORDER; ++i) {
		// printf("test: slab %d alloc/free\n", i);
		/* needs three slabs */
		cnt = SLAB_INIT_SIZE / powl(2, i) - 1;
		check_cnt = cnt -1;
		cnt = (cnt * 2 + 1);
		ptr = malloc(cnt * sizeof(void *));
		for (j = 0; j < cnt; ++j) {
			ptr[j] = alloc_in_slab(powl(2, i));
			mu_check(ptr[j] != NULL);
			/* slabs[i] only points to the newest slab.
			 * So, cannot execute:
			 * check_slab_range(slabs[i], ptr[j], j);
			 */
		}
		dump_slab(i, check_cnt);

		for (j = 0; j < cnt; ++j) {
			free_in_slab(ptr[j]);
		}
		check_cnt = (check_cnt + 1) * 3;
		dump_slab(i, check_cnt);
		free(ptr);
	}

}

MU_TEST_SUITE(test_suite)
{
	MU_RUN_TEST(test_buddy);
	MU_RUN_TEST(test_slab);
}

int main(int argc, char *argv[])
{
	MU_RUN_SUITE(test_suite);
	MU_REPORT();
	return minunit_status;
}
