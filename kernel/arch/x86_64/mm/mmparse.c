#include <common/types.h>
#include <common/kprint.h>
#include <common/macro.h>
#include <mm/mm.h>

#include "multiboot.h"

extern int physmem_map_num;
extern u64 physmem_map[8][2];

/* vaddr: kernel image end */
extern char img_end[];

/* Direct mapping in CHCORE */
extern char CHCORE_PUD_Direct_Mapping[];

#define SIZE_1G (1UL << 30)

#define PRESENT  (1 << 0)
#define WRITABLE (1 << 1)
#define HUGE_1G  (1 << 7)
#define GLOBAL   (1 << 8)
#define NX       (1UL << 63)

void setup_direct_mapping(u64 mem_size)
{
	u64 *direct_mapping;
	u64 idx;

	direct_mapping = (u64 *)CHCORE_PUD_Direct_Mapping;
	idx = 0;

	/* 0~1G has been mapped */
	while (mem_size > SIZE_1G) {

		direct_mapping += 1;
		idx += 1;

		/* Add mapping for 1G, 2G, 3G ... */
		*direct_mapping = (idx << 30) + PRESENT + WRITABLE
			                      + HUGE_1G + GLOBAL + NX;
		mem_size -= SIZE_1G;
	}
}

void parse_mem_map(void *info)
{
	u8 *p;
	u8 *ep;
	struct mbmem *temp;
	struct mbmem *mbmem;
	struct mbdata *mb;
	u64 mlength;

	paddr_t p_end;

	mb = (struct mbdata *)info;
	if (!(mb->flags & (1 << 6)))
		BUG("[x86_64] multiboot header has no memory map");

	p = (u8 *)phys_to_virt(mb->mmap_addr);
	ep = p + mb->mmap_length;

	mbmem = NULL;
	mlength = 0;
	while (p < ep) {
		temp = (struct mbmem *)p;
		p += 4 + temp->size;
		kinfo("e820: 0x%lx - 0x%lx, %s\n", temp->base,
		      temp->base + temp->length,
		      temp->type == 1 ? "usable" : "reserved");
		if ((temp->type == 1) && (temp->length > mlength)) {
			mbmem = temp;
			mlength = temp->length;
		}
	}

	if (mlength == 0) {
		BUG("Fails to detecting memory with Multiboot.\n");
		return;
	}

	// TODO: use the whole physical memory map from mb
	// use the last entry (biggest free chunk)
	physmem_map_num = 1;
	BUG_ON(mbmem->type != 1);

	/* remove kernel image part [0, img_end) */
	p_end = (u64)((void *)img_end - KCODE);
	if (mbmem->base < p_end)
		physmem_map[0][0] = p_end;
	else
		physmem_map[0][0] = mbmem->base;

	physmem_map[0][1] = mbmem->base + mbmem->length;
	setup_direct_mapping(physmem_map[0][1]);
	kinfo("Use memory: 0x%lx - 0x%lx\n", physmem_map[0][0],
					       physmem_map[0][1]);
}
