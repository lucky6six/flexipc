#include <stdio.h>
#include <chcore/bug.h>

#include "devtree.h"

extern const char binary_dtb_start;
extern const char binary_dtb_end;
extern const char binary_dtb_size;

#define kprint_padding(padding, fmt, ...) do {       \
	int __i;                                     \
	char __buff[256] __attribute__((unused));    \
	for (__i = 0; __i < padding; ++__i)          \
		__buff[__i] = ' ';                   \
	__buff[padding] = '\0';                      \
	debug("%s"fmt, __buff, ##__VA_ARGS__);       \
} while (0)

static inline void kprint_dev_node_compatible(int padding,
					      const char *compatible)
{
	char buff[256] __attribute__((unused));
	int i = 0;
	do {
		while (*compatible) {
			buff[i] = *compatible;
			++compatible;
			++i;
			BUG_ON(i > 256 - 2); /* invalid compatible format */
		}
		++compatible;
		buff[i] = ';';
		++i;
	} while (*compatible);

	buff[i] = '\0';
	kprint_padding(padding, "  %s: %s\n", "compatible", buff);
}

static inline void kprint_dev_node(struct dev_node *node, int padding)
{
	struct dev_node *child;

	// Begin the node
	kprint_padding(padding, "%s {\n", node->name);

	// Print properties
	kprint_dev_node_compatible(padding, node->compatible);
	kprint_padding(padding, "  %s: %s\n", "model", node->model);
	kprint_padding(padding, "  %s: <%d>\n", "phandle", node->phandle);
	kprint_padding(padding, "  %s: %s\n", "status",
		       node->status == DEV_STATUS_OK ? "okay" :
		       node->status == DEV_STATUS_DISABLED ? "disabled" :
		       node->status == DEV_STATUS_RESERVED? "reserved" :
		       node->status == DEV_STATUS_FAIL ? "fail" :
		       node->status == DEV_STATUS_NONE ? "none" : "fail-sss");
	kprint_padding(padding, "\n");

	// Print child nodes
	for (child = node->subdevices; child; child = child->next)
		kprint_dev_node(child, padding + 2);

	// End the node
	kprint_padding(padding, "}\n");
}

static inline void kprint_dev_rsvmaps(struct dev_tree *dt)
{
	int i;
	debug(" dtb: reserved memory regions:\n");
	for (i = 0; i < dt->rsvmem.nr_maps; ++i) {
		debug(" [0x%llx ~ 0x%llx]\n",
		      dt->rsvmem.maps[i].address,
		      dt->rsvmem.maps[i].address +
		      dt->rsvmem.maps[i].size - 1);
	}
}

void traverse_dev_node(struct dev_node *node, int (*cb)(struct dev_node *))
{
	struct dev_node *child;

	// call back with the current node
	cb(node);

	// Print child nodes
	for (child = node->subdevices; child; child = child->next)
		traverse_dev_node(child, cb);

}


extern struct dev_tree devtree;

void driver_init(const char *dtb_file_vaddr)
{

#if CONFIG_ARCH_AARCH64

	struct dev_tree *dtb_parse(struct dev_tree *dt, const char *dtb_addr);

	dtb_parse(&devtree, dtb_file_vaddr);
	info("dtb loaded\n");

	kprint_dev_rsvmaps(&devtree);
	kprint_dev_node(devtree.root, 0);

#else
	// arch_driver_init(&devtree);
	devtree.root = NULL;
	warn("Only aarch64 device tree is supported now\n");
	return;
#endif
}
