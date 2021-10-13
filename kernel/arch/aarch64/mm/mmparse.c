#include <arch/mmu.h>
#include <common/macro.h>
#include <common/types.h>
#include <mm/mm.h>

extern char img_end[];

#include "../../../mm/buddy_struct.h"

struct page;
extern int physmem_map_num;
extern u64 physmem_map[8][2];

#if CHCORE_PLAT == raspi3
	/* TODO: get usuable physical memory on Raspi3 */
	#define NPAGES (128*1024)
#else
	/* TODO: get usuable physical memory on Hikey */
	#define NPAGES (256*1024*4)
#endif

void parse_mem_map(void *info)
{
	u64 free_mem_start;

	/* A faked impl for now */
	free_mem_start = ROUND_UP((paddr_t)(&img_end), PAGE_SIZE);

	physmem_map[0][0] = free_mem_start;
	physmem_map[0][1] = free_mem_start + NPAGES * (PAGE_SIZE + sizeof(struct page));

	#if CHCORE_PLAT == raspi3
	#define SIZE_1G (1UL << 30)
	if (physmem_map[0][1] > SIZE_1G) {
		BUG("No enough physical memory on Raspi3: use smaller image or set less usuable pages\n");
	}
	#endif

	physmem_map_num = 1;
}
