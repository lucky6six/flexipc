#include <chcore/defs.h>
#include <stdio.h>
#include <stdlib.h>
#include <chcore/syscall.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <chcore/proc.h>
#include <chcore/ipc.h>
#include <chcore/fs_defs.h>
#include <chcore/elf.h>
#include <chcore/bug.h>
#include <chcore/launcher.h>
#include "devtree.h"


struct dev_tree devtree;

struct driver_module {
	const char *driver_name;
	const char *driver_path;
	const char *compatible;
};

#if 0
#define NR_DRIVER_MODULES (1)
struct driver_module driver_modules[NR_DRIVER_MODULES] = {
	{
		.driver_name = "UFS Driver",
		.driver_path = "/drivers/ufs-linux.srv",
		.compatible  = "hisilicon,kirin970-ufs"
	},
};
#else
#define NR_DRIVER_MODULES (0)
struct driver_module driver_modules[NR_DRIVER_MODULES] = { };
#endif

static inline
const struct driver_module *match_driver_module(struct dev_node *node)
{
	const char *compatible = node->compatible;
	int i;

	/* Try all compatible strings of the dev_node, in order. */
	while (*compatible) {

		for (i = 0; i < NR_DRIVER_MODULES; ++i) {
			struct driver_module *mod = &driver_modules[i];

			if (0 == strcmp(compatible, mod->compatible)) {
				return mod;
			}
		}

		while (*compatible) {
			++compatible;
		}
		++compatible;
	}
	return NULL;

}

#define DRVMOD_INFO_VADDR 0x10000000

int boot_driver(struct dev_node *node)
{
#if 0
	int info_pmo_cap;
	struct info_page *info_page;
	struct pmo_map_request *pmo_map_reqs;
	const struct driver_module *mod;
	int err;
	int ret;
	int i;

	mod = match_driver_module(node);
	if (mod == NULL) {
		/* No driver found for this device */
		return 0;
	}

	// TODO(MK): Check the status of the node

	// Prepare info page
	info_pmo_cap = usys_create_pmo(PAGE_SIZE, PMO_DATA);
	fail_cond(info_pmo_cap < 0, "usys_create_ret ret %d\n",
		  info_pmo_cap);

	ret = usys_map_pmo(SELF_CAP,
			   info_pmo_cap,
			   DRVMOD_INFO_VADDR, VM_READ | VM_WRITE);
	fail_cond(ret < 0, "usys_map_pmo ret %d\n", ret);

	info_page = (void *)DRVMOD_INFO_VADDR;
	info_page->ready_flag = false;
	info_page->nr_args = node->reg.nr_maps;


	/* One map for info_page, the rest for device pmo */
	pmo_map_reqs = malloc(sizeof(*pmo_map_reqs) * (1 + node->reg.nr_maps));

	pmo_map_reqs[0].pmo_cap = info_pmo_cap;
	pmo_map_reqs[0].addr = DRVMOD_INFO_VADDR;
	pmo_map_reqs[0].perm = VM_READ | VM_WRITE;

	for (i = 0; i < node->reg.nr_maps; ++i) {

		int pmo_cap;

		pmo_cap = usys_create_device_pmo(node->reg.maps[i].addr,
						 node->reg.maps[i].size);

		fail_cond(pmo_cap < 0, "usys_create_device_pmo failed %d\n",
			  pmo_cap);

		pmo_map_reqs[1 + i].pmo_cap = pmo_cap;
		pmo_map_reqs[1 + i].addr = pmo_map_reqs[i].addr + 0x40000;
			// (i ? node->reg.maps[i - 1].size : PAGE_SIZE);
		pmo_map_reqs[1 + i].perm = VM_READ | VM_WRITE;

		info_page->args[i] = pmo_map_reqs[1 + i].addr;

	}

	info("booting driver %s [%s]\n", mod->driver_name, mod->driver_path);

	// Start the driver module
	err = launch_process_path(mod->driver_path, NULL, pmo_map_reqs,
				  1 + node->reg.nr_maps, NULL, 0,
				  FIXED_BADGE_DRIVER_HUB);
	fail_cond(err != 0, "create_process returns %d\n", err);

	while (!info_page->ready_flag)
		usys_yield();

	/* TODO(MK): How to reclaim the pmos? */
#endif
	return 0;
}

void boot_drivers(void)
{
	if (devtree.root)
		traverse_dev_node(devtree.root, boot_driver);
}

int main(int argc, char *argv[])
{
	// driver_init((void *)info_page->args[0]);

	info("booting drivers...\n");
	boot_drivers();
	info("drivers UP\n");

	return 0;
}
