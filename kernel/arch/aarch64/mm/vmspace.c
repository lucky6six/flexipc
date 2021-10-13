#include <common/types.h>
#include <mm/vmspace.h>

/* In cache.c */
void cache_setup(void);

void arch_mm_init(void)
{
	cache_setup();
}

void arch_vmspace_init(struct vmspace *vmspace)
{
	/* empty now for aarch64 */
}
