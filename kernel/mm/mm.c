#include <mm/mm.h>
#include <common/kprint.h>
#include <common/macro.h>

#include "buddy.h"
#include "slab.h"

extern void parse_mem_map(void *);
extern void arch_mm_init(void);

struct global_mem global_mem;
int physmem_map_num;
u64 physmem_map[8][2];

/*
 * Layout:
 *
 * | metadata (npages * sizeof(struct page)) | start_vaddr ... (npages * PAGE_SIZE) |
 *
 */

void mm_init(void *info)
{
	vaddr_t free_mem_start = 0;
	vaddr_t free_mem_end = 0;
	struct page *page_meta_start = NULL;
	u64 npages = 0;
	u64 start_vaddr = 0;

	physmem_map_num = 0;

	/* TODO:
	 * - currently, only use the last entry biggest free chunk on x86
	 * - currently, we do not know how to detect physical memory map on aarch64
	 */
	parse_mem_map(info);

	if (physmem_map_num == 1) {
		free_mem_start = phys_to_virt(physmem_map[0][0]);
		free_mem_end   = phys_to_virt(physmem_map[0][1]);

		npages = (free_mem_end - free_mem_start) /
			 (PAGE_SIZE + sizeof(struct page));
		start_vaddr = (free_mem_start + npages * sizeof(struct page));
		start_vaddr = ROUND_UP(start_vaddr, PAGE_SIZE);
		kdebug("[CHCORE] mm: free_mem_start is 0x%lx, free_mem_end is 0x%lx\n",
		       free_mem_start, free_mem_end);
	} else {
		BUG("Unsupported physmem_map_num\n");
	}

	page_meta_start = (struct page *)free_mem_start;
	kdebug("page_meta_start: 0x%lx, real_start_vadd: 0x%lx,"
	       "npages: 0x%lx, meta_page_size: 0x%lx\n",
	       page_meta_start, start_vaddr, npages, sizeof(struct page));

	/* buddy alloctor for managing physical memory */
	init_buddy(&global_mem, page_meta_start, start_vaddr, npages);

	/* slab alloctor for allocating small memory regions */
	init_slab();

	/* init PCID */
	arch_mm_init();

	/* Init POSIX-SHM info */
	extern void init_global_shm_namespace(void);
	init_global_shm_namespace();
}
