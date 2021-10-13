#pragma once

#include <object/object.h>
#include <common/list.h>
#include <common/types.h>
#include <common/bitops.h>
#include <common/kprint.h>
#include <common/macro.h>
#include <common/lock.h>
#include <arch/sync.h>

struct object_slot {
	u64 slot_id;
	struct cap_group *cap_group;
	/* TODO: As bitmap is used, isvalid should be removed. Leave here to debug */
	int isvalid;
	/* TODO: extern rights to a more general per-cap data storage of an object */
	u64 rights;
	struct object *object;
	/* link copied slots pointing to the same object */
	struct list_head copies;
	/*
	 * Protect matedata of the slot.
	 * Should not get table_guard when slot_guard is held to avoid deadlock.
	 */
	struct rwlock slot_guard;
};

#define BASE_OBJECT_NUM		BITS_PER_LONG
/* 1st cap is cap_group. 2nd cap is vmspace */
#define CAP_GROUP_OBJ_ID	0
#define VMSPACE_OBJ_ID		1
struct slot_table {
	unsigned int slots_size;
	struct object_slot **slots;
	/*
	 * if a bit in full_slots_bmp is 1, corresponding
	 * sizeof(unsigned long) bits in slots_bmp are all set
	 */
	unsigned long *full_slots_bmp;
	unsigned long *slots_bmp;
	 /* XXX: Protect mapping of slot_id to slot. Maybe RCU is more suitable */
	struct rwlock table_guard;
};

#define MAX_GROUP_NAME_LEN 63

struct cap_group {
	struct slot_table slot_table;

	/* Proctect thread_list and thread_cnt */
	struct lock threads_lock;
	struct list_head thread_list;
	/* The number of threads */
	int thread_cnt;

	/*
	 * Each process has a unique badge as a global identifier which
	 * is set by the system server, procmgr.
	 * Currently, badge is used as a client ID during IPC.
	 */
	u64 badge;

	/* Ensures the cap_group_exit function only be executed onece */
	int notify_recycler;

	/* Now is used for debugging */
	char cap_group_name[MAX_GROUP_NAME_LEN + 1];
};

#define current_cap_group (current_thread->cap_group)

/*
 * ATTENTION: These interfaces are for capability internal use.
 * As a cap user, check object.h for interfaces for cap.
 */
int alloc_slot_id(struct cap_group *cap_group);

static inline void free_slot_id(struct cap_group *cap_group, int slot_id)
{
	struct slot_table *slot_table = &cap_group->slot_table;
	clear_bit(slot_id, slot_table->slots_bmp);
	clear_bit(slot_id / BITS_PER_LONG, slot_table->full_slots_bmp);
	slot_table->slots[slot_id] = NULL;
}

static inline struct object_slot *get_slot(struct cap_group *cap_group,
					   int slot_id)
{
	if (slot_id < 0 || slot_id >= cap_group->slot_table.slots_size)
		return NULL;
	return cap_group->slot_table.slots[slot_id];
}

static inline void install_slot(struct cap_group *cap_group, int slot_id,
				struct object_slot *slot)
{
	// BUG_ON(!is_write_locked(&cap_group->slot_table.table_guard));
	BUG_ON(!get_bit(slot_id, cap_group->slot_table.slots_bmp));
	cap_group->slot_table.slots[slot_id] = slot;
}

void *get_opaque(struct cap_group *cap_group, int slot_id, bool type_valid, int type);

int __cap_free(struct cap_group *cap_group, int slot_id,
	       bool slot_table_locked, bool copies_list_locked);

struct cap_group *create_root_cap_group(char *, size_t);

void sys_exit_group(int);

#define ROOT_CAP_GROUP_BADGE (0x1) /* INIT */
#define FSM_BADGE            (0x2)
#define PROCMGR_BADGE        (0x4)
