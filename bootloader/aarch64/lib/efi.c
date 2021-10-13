#include <lib/efi.h> /* in kernel/include/lib/efi.h */
#include <sort.h>
#include <boot.h>
#include <printf.h>

/* Global variables */
efi_memory_desc_t phy_map[EFI_MEMMAP_SIZE];
struct efi_memory_map phy_memmap;

efi_system_table_t *efi_sys_table;

void efi_error(char *str, efi_status_t status)
{
	printf("[EFI] Fatal Error: %s status %d\n", str, status);
	HLT;
}

char efi_memtypestr[EFI_MAX_MEMORY_TYPE + 1][40] = {
	"EFI_ReservedMemoryType",
	"EFI_LoaderCode",
	"EFI_LoaderData",
	"EFI_BootServicesCode",
	"EFI_BootServicesData",
	"EFI_RuntimeServicesCode",
	"EFI_RuntimeServicesData",
	"EFI_ConventionalMemory",
	"EFI_UnusableMemory",
	"EFI_ACPIReclaimMemory",
	"EFI_ACPIMemoryNVS",
	"EFI_MemoryMappedIO",
	"EFI_MemoryMappedIOPortSpace",
	"EFI_PalCode",
	"EFI_Presistentmemory",
	"EFI_Maxmemorytype"
};

void *simple_memcpy(void *dst, const void *src, size_t len)
{
	void *ret = dst;

	if (NULL == dst || NULL == src)
		return NULL;

	if (dst <= src || (char *)dst >= (char *)src + len) {
		while (len--) {
			*(char *)dst = *(char *)src;
			dst = (char *)dst + 1;
			src = (char *)src + 1;
		}
	} else {
		src = (char *)src + len - 1;
		dst = (char *)dst + len - 1;
		while (len--) {
			*(char *)dst = *(char *)src;
			dst = (char *)dst - 1;
			src = (char *)src - 1;
		}
	}
	return ret;
}

void simple_memset(char *dst, char ch, unsigned long len)
{
	unsigned long i;

	for (i = 0; i < len; ++i)
		dst[i] = ch;
}

static inline bool mmap_has_headroom(unsigned long buff_size,
				     unsigned long map_size,
				     unsigned long desc_size)
{
	unsigned long slack = buff_size - map_size;

	return slack / desc_size >= EFI_MMAP_NR_SLACK_SLOTS;
}

static int cmp_mem_desc(const void *l, const void *r)
{
	const efi_memory_desc_t *left = l, *right = r;

	return (left->phys_addr > right->phys_addr) ? 1 : -1;
}

/*
 * Returns whether region @left ends exactly where region @right starts,
 * or false if either argument is NULL.
 */
static bool regions_are_adjacent(efi_memory_desc_t *left,
				 efi_memory_desc_t *right)
{
	u64 left_end;

	if (left == NULL || right == NULL)
		return false;

	left_end = left->phys_addr + left->num_pages * EFI_PAGE_SIZE;

	return left_end == right->phys_addr;
}

/*
 * Returns whether region @left and region @right have compatible memory type
 * mapping attributes, and are both EFI_MEMORY_RUNTIME regions.
 */
static bool regions_have_compatible_memory_type_attrs(efi_memory_desc_t *left,
						      efi_memory_desc_t *right)
{
	static const u64 mem_type_mask = EFI_MEMORY_WB | EFI_MEMORY_WT |
	    EFI_MEMORY_WC | EFI_MEMORY_UC | EFI_MEMORY_RUNTIME;
	return ((left->attribute ^ right->attribute) & mem_type_mask) == 0;
}

/*
 * efi_get_virtmap() - create a virtual mapping for the EFI memory map
 *
 * This function populates the virt_addr fields of all memory region descriptors
 * in @memory_map whose EFI_MEMORY_RUNTIME attribute is set. Those descriptors
 * are also copied to @runtime_map, and their total count is returned in @count.
 */
void efi_get_virtmap(efi_memory_desc_t *memory_map, unsigned long map_size,
		     unsigned long desc_size, efi_memory_desc_t *runtime_map,
		     int *count)
{
	u64 efi_virt_base = EFI_RT_VIRTUAL_BASE;
	efi_memory_desc_t *in, *prev = NULL, *out = runtime_map;
	int l;
	u64 paddr, size;

	/*
	 * To work around potential issues with the Properties Table feature
	 * introduced in UEFI 2.5, which may split PE/COFF executable images
	 * in memory into several RuntimeServicesCode and RuntimeServicesData
	 * regions, we need to preserve the relative offsets between adjacent
	 * EFI_MEMORY_RUNTIME regions with the same memory type attributes.
	 * The easiest way to find adjacent regions is to sort the memory map
	 * before traversing it.
	 */
	sort(memory_map, map_size / desc_size, desc_size, cmp_mem_desc, 0);
	for (l = 0; l < map_size; l += desc_size, prev = in) {
		in = (void *)memory_map + l;
		if (!(in->attribute & EFI_MEMORY_RUNTIME))
			continue;
		paddr = in->phys_addr;
		size = in->num_pages * EFI_PAGE_SIZE;

		/*
		 * Make the mapping compatible with 64k pages: this allows
		 * a 4k page size kernel to kexec a 64k page size kernel and
		 * vice versa.
		 */
		if (!regions_are_adjacent(prev, in) ||
		    !regions_have_compatible_memory_type_attrs(prev, in)) {

			paddr = round_down(in->phys_addr, SZ_64K);
			size += in->phys_addr - paddr;

			/*
			 * Avoid wasting memory on PTEs by choosing a virtual
			 * base that is compatible with section mappings if this
			 * region has the appropriate size and physical
			 * alignment. (Sections are 2 MB on 4k granule kernels)
			 */
			if (IS_ALIGNED(in->phys_addr, SZ_2M) && size >= SZ_2M)
				efi_virt_base = round_up(efi_virt_base, SZ_2M);
			else
				efi_virt_base = round_up(efi_virt_base, SZ_64K);
		}

		in->virt_addr = efi_virt_base + in->phys_addr - paddr;
		efi_virt_base += size;
		simple_memcpy(out, in, desc_size);
		out = (void *)out + desc_size;
		++*count;
	}
}

static efi_status_t exit_boot_func(efi_system_table_t *sys_table_arg,
				   struct efi_boot_memmap *map, void *priv)
{
	struct exit_boot_struct *p = priv;
	/*
	 * Update the memory map with virtual addresses. The function will also
	 * populate @runtime_map with copies of just the EFI_MEMORY_RUNTIME
	 * entries so that we can pass it straight to SetVirtualAddressMap()
	 */
	efi_get_virtmap(*map->map, *map->map_size, *map->desc_size,
			p->runtime_map, p->runtime_entry_count);
	return 0;
}

efi_status_t efi_get_phy_memory_map(efi_system_table_t *sys_table_arg)
{
	efi_status_t status;
	unsigned long key;
	unsigned long map_size;
	unsigned long desc_size;
	u32 desc_version;

	desc_size = sizeof(efi_memory_desc_t);
	map_size = desc_size * EFI_MEMMAP_SIZE;

	status = efi_call_early(get_memory_map,
				&map_size,
				phy_map, &key, &desc_size, &desc_version);
	if (status == EFI_BUFFER_TOO_SMALL) {
		efi_error("memmap buf EFI_MEMMAP_SIZE too small\r\n", status);
	}

	phy_memmap.map = phy_map;
	phy_memmap.map_end = (void *)phy_map + map_size;
	phy_memmap.desc_size = desc_size;
	phy_memmap.desc_version = desc_version;
	return status;
}

efi_status_t efi_get_memory_map(efi_system_table_t *sys_table_arg,
				struct efi_boot_memmap *map)
{
	efi_memory_desc_t *m = NULL;
	efi_status_t status;
	unsigned long key;
	u32 desc_version;

	*map->desc_size = sizeof(*m);
	*map->map_size = *map->desc_size * 32;
	*map->buff_size = *map->map_size;
again:
	status = efi_call_early(allocate_pool, EFI_LOADER_DATA,
				*map->map_size, (void **)&m);
	if (status != EFI_SUCCESS)
		goto fail;
	*map->desc_size = 0;
	key = 0;
	status = efi_call_early(get_memory_map, map->map_size, m,
				&key, map->desc_size, &desc_version);
	if (status == EFI_BUFFER_TOO_SMALL ||
	    !mmap_has_headroom(*map->buff_size, *map->map_size,
			       *map->desc_size)) {
		efi_call_early(free_pool, m);
		/*
		 * Make sure there is some entries of headroom so that the
		 * buffer can be reused for a new map after allocations are
		 * no longer permitted.  Its unlikely that the map will grow to
		 * exceed this headroom once we are ready to trigger
		 * ExitBootServices()
		 */
		*map->map_size += *map->desc_size * EFI_MMAP_NR_SLACK_SLOTS;
		*map->buff_size = *map->map_size;
		goto again;
	}

	if (status != EFI_SUCCESS)
		efi_call_early(free_pool, m);

	if (map->key_ptr && status == EFI_SUCCESS)
		*map->key_ptr = key;
	if (map->desc_ver && status == EFI_SUCCESS)
		*map->desc_ver = desc_version;

fail:
	*map->map = m;
	return status;
}

/*
 * Handle calling ExitBootServices according to the requirements set out by the
 * spec.  Obtains the current memory map, and returns that info after calling
 * ExitBootServices.  The client must specify a function to perform any
 * processing of the memory map data prior to ExitBootServices.  A client
 * specific structure may be passed to the function via priv.  The client
 * function may be called multiple times.
 */
efi_status_t efi_exit_boot_services(efi_system_table_t *sys_table_arg,
				    void *handle,
				    struct efi_boot_memmap *map,
				    void *priv,
				    efi_exit_boot_map_processing priv_func)
{
	efi_status_t status;

	status = efi_get_memory_map(sys_table_arg, map);

	if (status != EFI_SUCCESS)
		goto fail;

	status = priv_func(sys_table_arg, map, priv);
	if (status != EFI_SUCCESS)
		goto free_map;

	status = efi_call_early(exit_boot_services, handle, *map->key_ptr);

	if (status == EFI_INVALID_PARAMETER) {
		/*
		 * The memory map changed between efi_get_memory_map() and
		 * exit_boot_services().  Per the UEFI Spec v2.6, Section 6.4:
		 * EFI_BOOT_SERVICES.ExitBootServices we need to get the
		 * updated map, and try again.  The spec implies one retry
		 * should be sufficent, which is confirmed against the EDK2
		 * implementation.  Per the spec, we can only invoke
		 * get_memory_map() and exit_boot_services() - we cannot alloc
		 * so efi_get_memory_map() cannot be used, and we must reuse
		 * the buffer.  For all practical purposes, the headroom in the
		 * buffer should account for any changes in the map so the call
		 * to get_memory_map() is expected to succeed here.
		 */
		*map->map_size = *map->buff_size;
		status = efi_call_early(get_memory_map,
					map->map_size,
					*map->map,
					map->key_ptr,
					map->desc_size, map->desc_ver);

		/* exit_boot_services() was called, thus cannot free */
		if (status != EFI_SUCCESS)
			goto fail;

		status = priv_func(sys_table_arg, map, priv);
		/* exit_boot_services() was called, thus cannot free */
		if (status != EFI_SUCCESS)
			goto fail;

		status =
		    efi_call_early(exit_boot_services, handle, *map->key_ptr);
	}

	/* exit_boot_services() was called, thus cannot free */
	if (status != EFI_SUCCESS)
		goto fail;

	return EFI_SUCCESS;

free_map:
	efi_call_early(free_pool, *map->map);
fail:
	return status;
}

void efi_free(efi_system_table_t *sys_table_arg, unsigned long size,
	      unsigned long addr)
{
	unsigned long nr_pages;

	if (!size)
		return;

	nr_pages = round_up(size, EFI_ALLOC_ALIGN) / EFI_PAGE_SIZE;
	efi_call_early(free_pages, addr, nr_pages);
}

efi_status_t exit_boot(efi_system_table_t *sys_table, void *handle)
{
	unsigned long map_size = 0;
	unsigned long buff_size;
	unsigned long desc_size = 0;
	u32 desc_ver;
	unsigned long mmap_key;
	efi_memory_desc_t *memory_map = NULL;
	efi_memory_desc_t *runtime_map = NULL;
	efi_status_t status;
	int runtime_entry_count = 0;
	struct efi_boot_memmap map;
	struct exit_boot_struct priv;

	printf("[EFI] Exiting boot service.\r\n");

	map.map = &runtime_map;
	map.map_size = &map_size;
	map.desc_size = &desc_size;
	map.desc_ver = &desc_ver;
	map.key_ptr = &mmap_key;
	map.buff_size = &buff_size;

	/*
	 * Get a copy of the current memory map that we will use to prepare
	 * the input for SetVirtualAddressMap(). We don't have to worry about
	 * subsequent allocations adding entries, since they could not affect
	 * the number of EFI_MEMORY_RUNTIME regions.
	 */
	status = efi_get_memory_map(sys_table, &map);
	if (status != EFI_SUCCESS) {
		printf("Unable to retrieve UEFI memory map.\r\n");
		return status;
	}
	map.map = &memory_map;
	priv.runtime_map = runtime_map;
	priv.runtime_entry_count = &runtime_entry_count;
	status = efi_exit_boot_services(sys_table, handle, &map, &priv,
					exit_boot_func);
	if (status == EFI_SUCCESS) {
		efi_set_virtual_address_map_t *svam;
		/* Install the new virtual address map */
		svam = sys_table->runtime->set_virtual_address_map;
		status = svam(runtime_entry_count * desc_size, desc_size,
			      desc_ver, runtime_map);
		/*
		 * We are beyond the point of no return here, so if the call to
		 * SetVirtualAddressMap() failed, we need to signal that to the
		 * incoming kernel but proceed normally otherwise.
		 */
		if (status != EFI_SUCCESS) {
			int l;
			/*
			 * Set the virtual address field of all
			 * EFI_MEMORY_RUNTIME entries to 0. This will signal
			 * the incoming kernel that no virtual translation has
			 * been installed.
			 */
			for (l = 0; l < map_size; l += desc_size) {
				efi_memory_desc_t *p = (void *)memory_map + l;

				if (p->attribute & EFI_MEMORY_RUNTIME)
					p->virt_addr = 0;
			}
		}
		return EFI_SUCCESS;
	}
	printf("[EFI] Exit boot services failed.\n");
	sys_table->boottime->free_pool(runtime_map);
	return EFI_LOAD_ERROR;
}

efi_status_t handle_kernel_image(efi_system_table_t *sys_table_arg,
				 unsigned long *image_addr,
				 unsigned long *image_size,
				 unsigned long *reserve_addr,
				 unsigned long *reserve_size,
				 unsigned long dram_base,
				 efi_loaded_image_t *image)
{
	efi_status_t status;
	unsigned long kernel_memsize = 0;
	unsigned long kernel_bss_size = 0;
	void *old_image_addr = (void *)*image_addr;
	unsigned long preferred_offset;

	preferred_offset = round_down(dram_base, MIN_KIMG_ALIGN) + TEXT_OFFSET;
	if (preferred_offset < dram_base)
		preferred_offset += MIN_KIMG_ALIGN;

	kernel_memsize = (&img_end - &img_start);
	kernel_bss_size = (&_bss_end - &_bss_start);

	if (*image_addr == preferred_offset)
		return EFI_SUCCESS;

	*image_addr = *reserve_addr = preferred_offset;
	*reserve_size = round_up(kernel_memsize, EFI_ALLOC_ALIGN);

	status = efi_call_early(allocate_pages, EFI_ALLOCATE_ANY_PAGES,
				EFI_LOADER_DATA,
				*reserve_size / EFI_PAGE_SIZE,
				(efi_physical_addr_t *) reserve_addr);

	printf("handle kernel image: alloc %d pages at 0x%lx\r\n",
	       *reserve_size / EFI_PAGE_SIZE, *reserve_addr);
	*image_addr = *reserve_addr;

	if (status == 0) {
		printf
		    ("kernel image: memmove from 0x%lx to 0x%lx size %lu\r\n",
		     old_image_addr, *image_addr, kernel_memsize);
		simple_memcpy((void *)*image_addr, old_image_addr, kernel_memsize);
		simple_memset((char *)&_bss_start, 0, kernel_bss_size);
	} else {
		printf("handle kernel image: alloc failed\r\n");
	}

	printf("handle kernel image: succ load image to 0x%lx\r\n",
	       *reserve_addr);
	return status;
}

void print_mem_attr(u64 attr)
{
	if (attr & EFI_MEMORY_UC) {
		printf("uncached ");
	}

	if (attr & EFI_MEMORY_WC) {
		printf("write-coalescing ");
	}

	if (attr & EFI_MEMORY_WT) {
		printf("write-through ");
	}

	if (attr & EFI_MEMORY_WB) {
		printf("write-back ");
	}

	if (attr & EFI_MEMORY_UCE) {
		printf("uncached, exported ");
	}

	if (attr & EFI_MEMORY_WP) {
		printf("write-protect ");
	}

	if (attr & EFI_MEMORY_RP) {
		printf("read-protect ");
	}

	if (attr & EFI_MEMORY_XP) {
		printf("execute-protect ");
	}

	if (attr & EFI_MEMORY_NV) {
		printf("non-volatile ");
	}

	if (attr & EFI_MEMORY_MORE_RELIABLE) {
		printf("higher reliability ");
	}

	if (attr & EFI_MEMORY_RO) {
		printf("read-only");
	}

	printf("\r\n");
}

struct chcore_memory_map *global_map;

void move_memmap(unsigned long dram_base)
{
	struct efi_memory_map *map = &phy_memmap;
	efi_memory_desc_t *md;
	efi_status_t status;
	int i = 0;
	void *start_addr = (void *)TEXT_OFFSET;
	size_t kernel_size = &img_end - &img_start;
	unsigned reserve_size = round_up(sizeof(struct chcore_memory_map), EFI_ALLOC_ALIGN);

	/* TODO: failed to save memory map after exit boot (value has been modified after exit) */
	global_map = (struct chcore_memory_map *)(start_addr + kernel_size);
	status = efi_call_early(allocate_pages, EFI_ALLOCATE_ANY_PAGES,
				EFI_LOADER_DATA,
				reserve_size/EFI_PAGE_SIZE,
				(efi_physical_addr_t *) global_map);
	if (status != EFI_SUCCESS)
		efi_error("Allocated memory map failed!", status);
	printf("Allocted pages at %lx\r\n", global_map);
	for_each_efi_memory_desc_in_map(map, md) {
		/* Move all desc into continuous space */
		global_map->descs[i].start_addr = md->phys_addr;
		global_map->descs[i].end_addr = md->phys_addr + md->num_pages * EFI_PAGE_SIZE;
		global_map->descs[i].type = md->type;
		global_map->descs[i].attribute = md->attribute;
		printf("[DESC 0x%lx] 0x%lx ~ 0x%lx\r\n", &global_map -> descs[i], global_map -> descs[i].start_addr, global_map -> descs[i].end_addr);
		if ((i ++) >= MAX_DESC)
			efi_error("CHCORE memory desc overflow\n\r", i);
	}
}

efi_status_t get_dram_base(efi_system_table_t *sys_table,
			   unsigned long *dram_base)
{
	efi_status_t status;
	struct efi_boot_memmap boot_map;
	unsigned long map_size = 0;
	unsigned long buff_size;
	struct efi_memory_map map = {0};
	efi_memory_desc_t *md;
	unsigned long key = 0;
	u32 desc_ver;
	unsigned long base = 0xffffffffffffffff;

	boot_map.map = (efi_memory_desc_t **) &map.map;
	boot_map.map_size = &map_size;
	boot_map.desc_size = &map.desc_size;
	boot_map.desc_ver = &desc_ver;
	boot_map.key_ptr = &key;
	boot_map.buff_size = &buff_size;

	status = efi_get_memory_map(sys_table, &boot_map);
	map.map_end = map.map + map_size;
	printf("get memory map status %d key %lx\r\n", status, key);
	if (status == EFI_SUCCESS) {
		for_each_efi_memory_desc_in_map(&map, md) {
#if 0
			printf("physical addr 0x%lx ~ 0x%lx %s\r\n",
			       md->phys_addr,
			       md->phys_addr + md->num_pages * EFI_PAGE_SIZE,
			       efi_memtypestr[md->type]);
			printf("\t\tpage %lu size %luMB attri: ",
			       md->num_pages,
			       md->num_pages * EFI_PAGE_SIZE / 1024 / 1024);
			print_mem_attr(md->attribute);
#endif
			if (base > md->phys_addr)
				base = md->phys_addr;
		}
		*dram_base = base;
	}
	status = efi_call_early(free_pool, (void *)map.map);
	return status;
}
