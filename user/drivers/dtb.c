/**
 * Accoding to Devicetree Specification (Release v0.2-21-g15cd53d)
 * by devicetree.org
**/

#include <endian.h>
#include <errno.h>
#include <chcore/error.h>
#include <stdlib.h>
#include <chcore/type.h>
#include <chcore/bug.h>
#include "devtree.h"


#define be128ptr_to_cpu_hi(x) (be64toh(*(u64 *)(x)))
#define be128ptr_to_cpu_lo(x) (be64toh(*((u64 *)(x) + 1)))

#define be96ptr_to_cpu_hi(x) (be32toh(*(u32 *)(x)))
#define be96ptr_to_cpu_lo(x) (((u64)(be32toh(*((u32 *)(x) + 1)))) << 32 | \
			      (be32toh(*((u32 *)(x)) + 2)))

#define beXptr_to_cpu(bits, n) \
	({ \
	 (bits == 32) ? be32toh(*(u32 *)(n)) : \
	 (bits == 64) ? be64toh(*(u64 *)(n)) : \
	 ({ BUG("invalid X"); 0; }); \
	 })

#define set_beXptr_to_cpu(bits, n, hi, lo) do {     \
	if (bits > 64) {                            \
		if (bits == 96) {                   \
			lo = be96ptr_to_cpu_lo(n);  \
			hi = be96ptr_to_cpu_hi(n);  \
		} else if (bits == 128) {           \
			lo = be128ptr_to_cpu_lo(n); \
			hi = be128ptr_to_cpu_hi(n); \
		} else {                            \
			BUG("invalid X");           \
		}                                   \
	} else {                                    \
		lo = 0;                             \
		hi = beXptr_to_cpu(bits, n);        \
	}                                           \
} while (0)

struct fdt_header {
	uint32_t magic;
	uint32_t totalsize;
	uint32_t off_dt_struct;
	uint32_t off_dt_strings;
	uint32_t off_mem_rsvmap;
	uint32_t version;
	uint32_t last_comp_version;
	uint32_t boot_cpuid_phys;
	uint32_t size_dt_strings;
	uint32_t size_dt_struct;
};


#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009

#define BYTE_AT(o,x) (*(((char *)(x)+(o))))

#define ALIGNUP_4B(ptr) ((typeof(ptr))(((uint64_t)(ptr) + 0x3) & ~0x3))
static inline void skip_string(uint32_t **token)
{
	while (BYTE_AT(0, *token) != 0 &&
	       BYTE_AT(1, *token) != 0 &&
	       BYTE_AT(2, *token) != 0 &&
	       BYTE_AT(3, *token) != 0)
		++(*token);
	++(*token);
}
static inline void skip_value(uint32_t **token, uint32_t value_len)
{
	*token = (uint32_t *)(ALIGNUP_4B((uint64_t)(*token) + value_len));
}

static inline struct dev_node *new_dev_node(struct dev_node *parent)
{
	struct dev_node *node = NULL;

	node = malloc(sizeof(*node));

	node->next = NULL;
	node->parent = parent;
	node->subdevices = NULL;

	node->name[0] = '\0';
	node->compatible[0] = node->compatible[1] = '\0'; /* Two '\0' are necessary for multi strings */
	node->model[0] = '\0';
	node->phandle = 0;
	node->status = 0;

	node->reg.nr_maps = 0;
	node->reg.maps = NULL;
	node->reg.virtual_reg = 0;

	node->ranges.has_ranges = false;
	node->ranges.has_dma_ranges = false;
	node->ranges.nr_ranges = 0;
	node->ranges.ranges = NULL;
	node->ranges.nr_dma_ranges = 0;
	node->ranges.dma_ranges = NULL;

	/* Two default values from the manual */
	node->_address_cells = 2;
	node->_size_cells = 1;

	node->_interrupt_cells = 0;

	return node;
}

static inline size_t kstrncpy(char *dst, size_t max, const char *src)
{
	size_t i;

	for (i = 0; i < max - 1 && *src; ++i) {
		*dst = *src;
		++dst; ++src;
	}
	*dst = '\0';

	return i;
}

static inline void kstrncpy_alert(char *dst, size_t max, const char *src)
{
	size_t i = kstrncpy(dst, max, src);

	if (src[i])
		BUG("too long string in kstrncpy!\n");
}

/**
 * Compare the first @len chars of two strings. The whole strings are compared
 * if @len is -1.
 */
static inline int kstrncmp(const char *str1, const char *str2, size_t len)
{
	for ( ; len--; ++str1, ++str2) {
		if (likely(*str1 && *str2)) {
			if (*str1 == *str2)
				continue;
			return (*str1 < *str2) ? -1 : 1;
		}
		if (!*str1 && !*str2)
			break;
		return (*str1) ? 1 : -1;
	}
	return 0;
}

static inline __attribute__((unused))
void khexdump(const char *addr, size_t len)
{
	int i;

	info("<==> DUMP 0x%lx ~ 0x%lx <==>\n", addr, addr + len);
	for (i = 0; i < len; i += 8) {
		if (len >= 8) {
			info("0x%lx: %x %x %x %x  %x %x %x %x\n",
			      addr + i,
			      addr[i + 0], addr[i + 1], addr[i + 2],
			      addr[i + 3], addr[i + 4], addr[i + 5],
			      addr[i + 6], addr[i + 7]);
		} else {
			if (len >= 4) {
				info("0x%lx: %x %x %x %x\n",
				      addr + i, addr[i + 0], addr[i + 1],
				      addr[i + 2], addr[i + 3]);
				i -= 4;
			}
			while (i < len) {
				info("0x%lx: %x\n", addr + i, addr[i]);
				--i;
			}
		}
	}
	return;
}

static inline int dtb_parse_property(struct dev_node *node, const char *name,
				     const void *value, size_t value_len)
{
	if (0 == kstrncmp(name, "compatible", -1)) {
		/* value_len should contains the last '\0'. */
		if (value_len + 1 > DEV_PROP_LEN) {
			/* Too large, how to handle? */
			BUG("string too long");
		}
		memcpy(node->compatible, value, value_len);
		node->compatible[value_len] = '\0';
		return 0;
	}
	if (0 == kstrncmp(name, "model", -1)) {
		/* FIXME(MK): Since model is one string, we could ignore
		 * value_len? */
		kstrncpy_alert(node->model, DEV_NAME_LEN, value);
		return 0;
	}
	if (0 == kstrncmp(name, "phandle", -1)) {
		node->phandle = be32toh(*(uint32_t *)value);
		return 0;
	}
	if (0 == kstrncmp(name, "status", -1)) {
		if (0 == kstrncmp(value, "okay", -1) ||
		    0 == kstrncmp(value, "ok", -1)) {
			node->status = DEV_STATUS_OK;
		} else if (0 == kstrncmp(value, "disabled", -1)) {
			node->status = DEV_STATUS_DISABLED;
		} else if (0 == kstrncmp(value, "reserved", -1)) {
			node->status = DEV_STATUS_RESERVED;
		} else if (0 == kstrncmp(value, "fail", -1)) {
			node->status = DEV_STATUS_FAIL;
		} else if (0 == kstrncmp(value, "fail-", 5)) {
			/* TODO(MK): We don't handle fail-XXX now. */
			BUG("fail-XXX not implemented");
		} else {
			info("%s\n", value);
			BUG("unknown device node status");
		}
		return 0;
	}
	if (0 == kstrncmp(name, "#address-cells", -1)) {
		node->_address_cells = be32toh(*(uint32_t *)value);
		return 0;
	}
	if (0 == kstrncmp(name, "#size-cells", -1)) {
		node->_size_cells = be32toh(*(uint32_t *)value);
		return 0;
	}
	if (0 == kstrncmp(name, "#interrupt-cells", -1)) {
		node->_interrupt_cells = be32toh(*(uint32_t *)value);
		return 0;
	}
	if (0 == kstrncmp(name, "reg", -1)) {
		u32 _address_cells;
		u32 _size_cells;
		u32 tuple_len;
		int i;

		/* reg depends on the *cells* defined in the parent node */
		BUG_ON(!node->parent);
		_address_cells = node->parent->_address_cells;
		_size_cells = node->parent->_size_cells;
		tuple_len = (_address_cells + _size_cells) * sizeof(u32);
		BUG_ON(value_len % tuple_len);

		/* We only support 1 (32-bit) and 2 (64-bit) addresses */
		BUG_ON(_address_cells != 1 && _address_cells != 2);
		/* We support 0, 1 (32-bit) and 2 (64-bit) sizes */
		BUG_ON(_size_cells > 2);

		/* init regmaps */
		node->reg.nr_maps = value_len / tuple_len;
		node->reg.maps = malloc(sizeof(*node->reg.maps) *
					node->reg.nr_maps);
		if (!node->reg.maps) {
			node->reg.nr_maps = 0;
			return -ENOMEM;
		}
		for (i = 0; i < node->reg.nr_maps; ++i) {
			node->reg.maps[i].addr = beXptr_to_cpu(_address_cells *
							       32,
							       value);
			value += _address_cells * sizeof(u32);
			if (!_size_cells) {
				node->reg.maps[i].size = 0;
				continue;
			}
			node->reg.maps[i].size = beXptr_to_cpu(_size_cells * 32,
							       value);
			value += _size_cells * sizeof(u32);
		}
		return 0;
	}
	if (0 == kstrncmp(name, "ranges", -1)) {
		u32 _child_address_cells;
		u32 _parent_address_cells;
		u32 _size_cells;
		u32 tuple_len;
		int i;

		node->ranges.has_ranges = true;
		if (!value_len)
			/* empty value, but ranges are specified */
			return 0;

		BUG_ON(!node->parent);
		/* node->parent->parent defines parent's #address-cells */
		BUG_ON(!node->parent->parent);
		/* This node defines child's #address-cells/#size-cells */
		_child_address_cells = node->_address_cells;
		_parent_address_cells = node->parent->parent->_address_cells;
		_size_cells = node->_size_cells;
		tuple_len = (_child_address_cells + _parent_address_cells +
			     _size_cells) * sizeof(u32);
		BUG_ON(value_len % tuple_len);

		/* We support 1 (32-bit), 2 (64-bit), 3 (96-bit) and
		 * 4 (128-bit) addresses. */
		BUG_ON(_child_address_cells < 0 || _child_address_cells > 4);
		BUG_ON(_parent_address_cells < 0 || _parent_address_cells > 4);
		/* We support 0, 1 (32-bit) and 2 (64-bit) sizes */
		BUG_ON(_size_cells > 2);

		/* init range maps */
		node->ranges.nr_ranges = value_len / tuple_len;
		node->ranges.ranges = malloc(sizeof(*node->ranges.ranges) *
					     node->ranges.nr_ranges);
		if (!node->ranges.ranges) {
			node->ranges.nr_ranges = 0;
			return -ENOMEM;
		}
		for (i = 0; i < node->ranges.nr_ranges; ++i) {
			/* child */
			set_beXptr_to_cpu(_child_address_cells * 32, value,
				node->ranges.ranges[i].child_bus_address_hi,
				node->ranges.ranges[i].child_bus_address_lo);
			value += _child_address_cells * sizeof(u32);
			/* parent */
			set_beXptr_to_cpu(_parent_address_cells * 32, value,
				node->ranges.ranges[i].parent_bus_address_hi,
				node->ranges.ranges[i].parent_bus_address_lo);
			value += _parent_address_cells * sizeof(u32);

			/* length */
			BUG_ON(_size_cells == 0);
			node->ranges.ranges[i].length =
				beXptr_to_cpu(_size_cells * 32, value);
			value += _size_cells * sizeof(u32);
		}
		return 0;
	}
	debug("unhandled property: %s\n", name);
	return -ENOTSUP;
}

int dtb_parse_struct_recursively(uint32_t **token, uint32_t *token_end,
				 const char *dt_strings,
				 struct dev_node *parent)
{
	struct dev_node *node = NULL;
	uint32_t value_len;
	const char *prop_name;
	const char *value;
	int err;

	while (*token < token_end) {

		switch (be32toh(**token)) {

		case FDT_NOP:
			++(*token);
			continue;

		case FDT_BEGIN_NODE:
			/* We have reached the beginning of a new node */
			node = new_dev_node(parent);

			++(*token);
			// Record the unit name
			/* FIXME(MK): What's the difference b/t name and unit
			 * name? */
			kstrncpy_alert(node->name, DEV_NAME_LEN,
				       (const char *)(*token));
			skip_string(token); /* Move to next token */

			// Parse the whole node recursively
			dtb_parse_struct_recursively(token, token_end,
						     dt_strings,
						     node);
			++(*token);
			// Push node to parent's child list
			node->next = parent->subdevices;
			parent->subdevices = node;
			break;

		case FDT_END_NODE:
			/* End of this node, return.
			 * We don't need to advance *token here because the
			 * caller will do that. */
			return 0;

		case FDT_PROP:

			BUG_ON(parent == NULL);

			++(*token); /* Move to value len */
			value_len = be32toh(**token);

			++(*token); /* Move to nameoff */
			prop_name = (const char *)dt_strings;
			prop_name += be32toh(**token);

			++(*token); /* Move to value */
			/* TODO(MK): The value should be further parsed */
			value = (const char *)(*token);

			skip_value(token, value_len); /* Move to next token */

			err = dtb_parse_property(parent, prop_name, value,
						 value_len);
			if (err == -ENOMEM)
				return -ENOMEM;
			break;

		case FDT_END:
			return 0;

		default:
			return -EINVAL;
		}
	}
	return -EINVAL;
}

struct fdt_reserve_entry {
	uint64_t address;
	uint64_t size;
};

struct dev_tree *dtb_parse(struct dev_tree *dt, const char *dtb_addr)
{
	struct fdt_header *header_be; /* used to parse big-endianness header */
	struct fdt_header header;
	struct fdt_reserve_entry *rsvent_be; /* used to parse rsvmap entry */
	struct dev_node top;
	uint32_t *token_end;
	uint32_t *token;
	int err;
	int i;

	// Parse header
	header_be = (struct fdt_header *)dtb_addr;
	header.magic = be32toh(header_be->magic);
	if (header.magic != 0xd00dfeed) {
		err = -EINVAL;
		goto err_magic;
	}
	header.totalsize = be32toh(header_be->totalsize);
	header.off_dt_struct = be32toh(header_be->off_dt_struct);
	header.off_dt_strings = be32toh(header_be->off_dt_strings);
	header.off_mem_rsvmap = be32toh(header_be->off_mem_rsvmap);
	header.version = be32toh(header_be->version);
	header.last_comp_version = be32toh(header_be->last_comp_version);
	header.boot_cpuid_phys = be32toh(header_be->boot_cpuid_phys);
	header.size_dt_strings = be32toh(header_be->size_dt_strings);
	header.size_dt_struct = be32toh(header_be->size_dt_struct);

	// Parse reserved memory regions
	rsvent_be = (struct fdt_reserve_entry *)(dtb_addr +
						 header.off_mem_rsvmap);
	dt->rsvmem.nr_maps = 0;
	while (rsvent_be->address || rsvent_be->size) {
		rsvent_be++;
		dt->rsvmem.nr_maps++;
	}

	dt->rsvmem.maps = malloc(sizeof(*dt->rsvmem.maps) * dt->rsvmem.nr_maps);
	rsvent_be = (struct fdt_reserve_entry *)(dtb_addr +
						 header.off_mem_rsvmap);
	for (i = 0; i < dt->rsvmem.nr_maps; ++i) {
		dt->rsvmem.maps[i].address = be64toh(rsvent_be->address);
		dt->rsvmem.maps[i].size = be64toh(rsvent_be->size);
		rsvent_be++;
	}

	// Parse struct bocks
	token_end = (uint32_t *)(dtb_addr + header.off_dt_struct +
				 header.size_dt_struct);

	top.subdevices = NULL;
	token = (uint32_t *)(dtb_addr + header.off_dt_struct);
	err = dtb_parse_struct_recursively(&token, token_end,
					   dtb_addr + header.off_dt_strings,
					   &top);
	if (err)
		return CHCORE_ERR_PTR(err);

	dt->root = top.subdevices;
	return dt;

err_magic:
	return CHCORE_ERR_PTR(err);
}


