#include <object/cap_group.h>
#include <object/thread.h>
#include <common/list.h>
#include <common/util.h>
#include <common/bitops.h>
#include <mm/kmalloc.h>
#include <mm/vmspace.h>
#include <mm/uaccess.h>
#include <lib/printk.h>
#include <ipc/notification.h>

/* tool functions */
static bool is_valid_slot_id(struct slot_table *slot_table, int slot_id)
{
	if (slot_id < 0 && slot_id >= slot_table->slots_size)
		return false;
	if (!get_bit(slot_id, slot_table->slots_bmp))
		return false;
	if (slot_table->slots[slot_id] == NULL)
		BUG("slot NULL while bmp is not\n");
	return true;
}

static int slot_table_init(struct slot_table *slot_table, unsigned int size,
			   bool init_lock)
{
	int r;

	size = DIV_ROUND_UP(size, BASE_OBJECT_NUM) * BASE_OBJECT_NUM;
	slot_table->slots_size = size;
	/* XXX: vmalloc is better? */
	slot_table->slots = kzalloc(size * sizeof(*slot_table->slots));
	if (!slot_table->slots) {
		r = -ENOMEM;
		goto out_fail;
	}

	slot_table->slots_bmp = kzalloc(BITS_TO_LONGS(size)
					* sizeof(unsigned long));
	if (!slot_table->slots_bmp) {
		r = -ENOMEM;
		goto out_free_slots;
	}

	slot_table->full_slots_bmp = kzalloc(BITS_TO_LONGS(BITS_TO_LONGS(size))
					     * sizeof(unsigned long));
	if (!slot_table->full_slots_bmp) {
		r = -ENOMEM;
		goto out_free_slots_bmp;
	}

	if (init_lock)
		rwlock_init(&slot_table->table_guard);

	return 0;
out_free_slots_bmp:
	kfree(slot_table->slots_bmp);
out_free_slots:
	kfree(slot_table->slots);
out_fail:
	return r;
}

int cap_group_init(struct cap_group *cap_group, unsigned int size, u64 badge)
{
	struct slot_table *slot_table = &cap_group->slot_table;

	BUG_ON(slot_table_init(slot_table, size, true));
	init_list_head(&cap_group->thread_list);
	lock_init(&cap_group->threads_lock);
	cap_group->thread_cnt = 0;

	/* Set badge of the new cap group. */
	cap_group->badge = badge;

	return 0;
}

/* slot allocation */
static int expand_slot_table(struct slot_table *slot_table)
{
	unsigned int new_size, old_size;
	struct slot_table new_slot_table;
	int r;

	old_size = slot_table->slots_size;
	new_size = old_size + BASE_OBJECT_NUM;
	r = slot_table_init(&new_slot_table, new_size, false);
	if (r < 0)
		return r;

	memcpy(new_slot_table.slots, slot_table->slots,
	       old_size * sizeof(*slot_table->slots));
	memcpy(new_slot_table.slots_bmp, slot_table->slots_bmp,
	       BITS_TO_LONGS(old_size) * sizeof(unsigned long));
	memcpy(new_slot_table.full_slots_bmp, slot_table->full_slots_bmp,
	       BITS_TO_LONGS(BITS_TO_LONGS(old_size)) * sizeof(unsigned long));
	slot_table->slots_size = new_size;
	kfree(slot_table->slots);
	slot_table->slots = new_slot_table.slots;
	kfree(slot_table->slots_bmp);
	slot_table->slots_bmp = new_slot_table.slots_bmp;
	kfree(slot_table->full_slots_bmp);
	slot_table->full_slots_bmp = new_slot_table.full_slots_bmp;
	return 0;
}

/* should only be called when table_guard is held */
int alloc_slot_id(struct cap_group *cap_group)
{
	int empty_idx = 0, r;
	struct slot_table *slot_table;
	int bmp_size = 0, full_bmp_size = 0;

	slot_table = &cap_group->slot_table;

	while (true) {
		bmp_size = slot_table->slots_size;
		full_bmp_size = BITS_TO_LONGS(bmp_size);

		empty_idx = find_next_zero_bit(slot_table->full_slots_bmp,
					       full_bmp_size, 0);
		if (empty_idx >= full_bmp_size)
			goto expand;

		empty_idx = find_next_zero_bit(slot_table->slots_bmp,
					       bmp_size,
					       empty_idx * BITS_PER_LONG);
		if (empty_idx >= bmp_size)
			goto expand;
		else
			break;
expand:
		r = expand_slot_table(slot_table);
		if (r < 0)
			goto out_fail;
	}
	BUG_ON(empty_idx < 0 || empty_idx >= bmp_size);

	set_bit(empty_idx, slot_table->slots_bmp);
	if (slot_table->full_slots_bmp[empty_idx / BITS_PER_LONG]
	    == ~((unsigned long)0))
		set_bit(empty_idx / BITS_PER_LONG, slot_table->full_slots_bmp);

	return empty_idx;
out_fail:
	return r;
}

void *get_opaque(struct cap_group *cap_group, int slot_id,
			bool type_valid, int type)
{
	struct slot_table *slot_table = &cap_group->slot_table;
	struct object_slot *slot;
	void *obj;

	read_lock(&slot_table->table_guard);
	if (!is_valid_slot_id(slot_table, slot_id)) {
		obj = NULL;
		goto out_unlock_table;
	}

	slot = get_slot(cap_group, slot_id);
	read_lock(&slot->slot_guard);
	BUG_ON(slot->isvalid == false);
	BUG_ON(slot->object == NULL);

	if (!type_valid || slot->object->type == type) {
		obj = slot->object->opaque;
	} else {
		obj = NULL;
		goto out_unlock_slot;
	}

	atomic_fetch_add_64(&slot->object->refcount, 1);

out_unlock_slot:
	read_unlock(&slot->slot_guard);
out_unlock_table:
	read_unlock(&slot_table->table_guard);
	return obj;
}

/* Get an object reference through its cap.
 * The interface will also add the object's refcnt by one.
 */
void *obj_get(struct cap_group *cap_group, int slot_id, int type)
{
	return get_opaque(cap_group, slot_id, true, type);
}

/* This is a pair interface of obj_get.
 * Used when no releasing an object reference.
 * The interface will minus the object's refcnt by one.
 *
 * Furthermore, free an object when its reference cnt becomes 0.
 */
void obj_put(void *obj)
{
	struct object *object;
	u64 old_refcount;

	object = container_of(obj, struct object, opaque);
	old_refcount = atomic_fetch_sub_64(&object->refcount, 1);

	if (old_refcount == 1)
		kfree(object);
}


/* An exposed syscall for creating a cap_group */
int sys_create_cap_group(u64 badge, u64 cap_group_name, u64 name_len)
{
	struct cap_group *new_cap_group;
	struct vmspace *vmspace;
	int cap, r;

	if ((current_cap_group->badge != ROOT_CAP_GROUP_BADGE) &&
	    (current_cap_group->badge != FSM_BADGE) &&
	    (current_cap_group->badge != PROCMGR_BADGE)) {
		kinfo("An unthorized process tries to create cap_group.\n");
		return -EPERM;
	}

	/* cap current cap_group */
	new_cap_group = obj_alloc(TYPE_CAP_GROUP, sizeof(*new_cap_group));
	if (!new_cap_group) {
		r = -ENOMEM;
		goto out_fail;
	}
	cap_group_init(new_cap_group, BASE_OBJECT_NUM, badge);

	cap = cap_alloc(current_cap_group, new_cap_group, 0);
	if (cap < 0) {
		r = cap;
		goto out_free_obj_new_grp;
	}

	/* 1st cap is cap_group */
	if (cap_copy(current_thread->cap_group, new_cap_group, cap, 0, 0)
	    != CAP_GROUP_OBJ_ID) {
		printk("init cap_group cap[0] is not cap_group\n");
		r = -1;
		goto out_free_cap_grp_current;
	}

	/* 2st cap is vmspace */
	vmspace = obj_alloc(TYPE_VMSPACE, sizeof(*vmspace));
	if (!vmspace) {
		r = -ENOMEM;
		goto out_free_obj_vmspace;
	}
	vmspace_init(vmspace);
	r = cap_alloc(new_cap_group, vmspace, 0);
	if (r < 0)
		goto out_free_obj_vmspace;
	else if (r != VMSPACE_OBJ_ID)
		BUG("init cap_group cap[1] is not vmspace\n");


	new_cap_group->notify_recycler = 0;

	/* Set the cap_group_name (process_name) for easing debugging */
	memset(new_cap_group->cap_group_name, 0, MAX_GROUP_NAME_LEN);
	if (name_len > MAX_GROUP_NAME_LEN)
		name_len = MAX_GROUP_NAME_LEN;
	copy_from_user(new_cap_group->cap_group_name,
		       (char *)cap_group_name, name_len);

	return cap;
out_free_obj_vmspace:
	obj_free(vmspace);
out_free_cap_grp_current:
	cap_free(current_cap_group, cap);
	new_cap_group = NULL;
out_free_obj_new_grp:
	obj_free(new_cap_group);
out_fail:
	return r;
}

/* This is for creating the first (init) user process. */
struct cap_group *create_root_cap_group(char *name, size_t name_len)
{
	struct cap_group *cap_group;
	struct object *object;
	struct object_slot *slot;
	struct vmspace *vmspace;
	int total_size, slot_id;

	total_size = sizeof(*object) + sizeof(*cap_group);
	if ((object = kmalloc(total_size)) == NULL)
		goto out_fail;
	object->type = TYPE_CAP_GROUP;
	object->size = sizeof(*cap_group);
	object->refcount = 1;
	cap_group = (struct cap_group *)object->opaque;

	cap_group_init(cap_group, BASE_OBJECT_NUM,
		       /* Fixed badge */ ROOT_CAP_GROUP_BADGE);

	// put the cap of the cap_group its self on the first slot
	slot_id = alloc_slot_id(cap_group);
	BUG_ON(slot_id != CAP_GROUP_OBJ_ID);
	slot = kzalloc(sizeof(*slot));
	if (!slot)
		goto out_free_cap_group;
	slot->slot_id = slot_id;
	slot->cap_group = cap_group;
	slot->isvalid = true;
	slot->object = object;
	init_list_head(&slot->copies);
	rwlock_init(&slot->slot_guard);
	cap_group->slot_table.slots[slot_id] = slot;

	vmspace = obj_alloc(TYPE_VMSPACE, sizeof(*vmspace));
	BUG_ON(!vmspace);
	vmspace_init(vmspace);
	slot_id = cap_alloc(cap_group, vmspace, 0);
	BUG_ON(slot_id != VMSPACE_OBJ_ID);


	/* Set the cap_group_name (process_name) for easing debugging */
	memset(cap_group->cap_group_name, 0, MAX_GROUP_NAME_LEN);
	if (name_len > MAX_GROUP_NAME_LEN)
		name_len = MAX_GROUP_NAME_LEN;
	memcpy(cap_group->cap_group_name, name, name_len);

	return cap_group;
out_free_cap_group:
	kfree(cap_group);
out_fail:
	return NULL;
}

