/**
 * Device tree manages all hardware devices on the platform.
 */
#pragma once

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define PREFIX "[driver_hub]"

#define info(fmt, ...) printf(PREFIX " " fmt, ## __VA_ARGS__)
// #define debug(fmt, ...) printf(PREFIX " " fmt, ## __VA_ARGS__)
#define debug(fmt, ...) do {} while (0)
#define error(fmt, ...) printf(PREFIX " " fmt, ## __VA_ARGS__)
#define warn(fmt, ...) printf(PREFIX " " fmt, ## __VA_ARGS__)

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint64_t paddr_t;
typedef uint64_t vaddr_t;

#define DEV_NAME_LEN 256
#define DEV_PROP_LEN 128

struct dev_regmap {
	paddr_t addr;
	size_t size;
};

struct dev_regmaps {
	size_t nr_maps;
	struct dev_regmap *maps;
	vaddr_t virtual_reg;
};

struct dev_range {
	paddr_t child_bus_address_hi;
	paddr_t child_bus_address_lo;
	paddr_t parent_bus_address_hi;
	paddr_t parent_bus_address_lo;
	size_t length;
};

struct dev_ranges {

	bool has_ranges;
	bool has_dma_ranges;

	size_t nr_ranges;
	struct dev_range *ranges;

	size_t nr_dma_ranges;
	struct dev_range *dma_ranges;
};

enum {
	DEV_STATUS_NONE = 0,
	DEV_STATUS_OK = 1,
	DEV_STATUS_DISABLED = 2,
	DEV_STATUS_RESERVED = 3,
	/* Reserve some values for future use */
	DEV_STATUS_FAIL = 10
	/* fail-sss should use the rest values starting with 11 */
};
struct dev_node {
	/* Properties */
	char name[DEV_NAME_LEN];
	char compatible[DEV_PROP_LEN]; /* compatible format is special: it
					  consists of multiple strings,
					  separated by '\0'. The end of the
					  last string is indicated by two '\0's.
					  */
	char model[DEV_PROP_LEN];
	u32 phandle;
	u32 status;
	struct dev_regmaps reg;
	struct dev_ranges ranges;
	/* The following two numbers are used in child nodes */
	u32 _address_cells;
	u32 _size_cells;
	u32 _interrupt_cells;

	/* Hierarchy */
	struct dev_node *parent;
	struct dev_node *next; /* Next dev_node under the same parent node */
	struct dev_node *subdevices; /* Child devices of this node */
};

struct dev_reserve_memmap {
	paddr_t address;
	size_t size;
};

struct dev_reserve_memmaps {
	size_t nr_maps;
	struct dev_reserve_memmap *maps;
};

struct dev_tree {
	struct dev_node *root;
	struct dev_reserve_memmaps rsvmem;
};

void driver_init(const char *dtb_file_vaddr);
void traverse_dev_node(struct dev_node *node, int (*cb)(struct dev_node *));
