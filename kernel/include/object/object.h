#pragma once

#include <common/types.h>
#include <common/errno.h>
#include <common/lock.h>
#include <common/list.h>
#include <common/lock.h>

struct object {
	u64 type;
	u64 size;
	/* Link all slots point to this object */
	struct list_head copies_head;
	/* Currently only protect copies list */
	struct lock copies_lock;
	/*
	 * refcount is added when a slot points to it and when get_object is
	 * called. Object is freed when it reaches 0.
	 */
	u64 refcount;
	u64 opaque[];
};

enum object_type {
	TYPE_CAP_GROUP = 0,
	TYPE_THREAD,
	TYPE_CONNECTION,
	TYPE_NOTIFICATION,
	TYPE_IRQ,
	TYPE_PMO,
	TYPE_VMSPACE,
	TYPE_NR,
};

struct cap_group;

typedef void (*obj_deinit_func)(void *);
extern const obj_deinit_func obj_deinit_tbl[TYPE_NR];

void *obj_get(struct cap_group *cap_group, int slot_id, int type);
void obj_put(void *obj);

void *obj_alloc(u64 type, u64 size);
void obj_free(void *obj);
int cap_alloc(struct cap_group *cap_group, void *obj, u64 rights);
int cap_free(struct cap_group *cap_group, int slot_id);
int cap_copy(struct cap_group *src_cap_group, struct cap_group *dest_cap_group,
	     int src_slot_id, bool new_rights_valid, u64 new_rights);
int cap_copy_local(struct cap_group *cap_group, int src_slot_id, u64 new_rights);
int cap_move(struct cap_group *src_cap_group, struct cap_group *dest_cap_group,
	     int src_slot_id, bool new_rights_valid, u64 new_rights);
int cap_revoke(struct cap_group *cap_group, int slot_id);
