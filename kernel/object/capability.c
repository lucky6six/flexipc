#include <object/object.h>
#include <object/cap_group.h>
#include <object/endpoint.h>
#include <object/thread.h>
#include <object/irq.h>
#include <mm/kmalloc.h>
#include <mm/uaccess.h>
#include <lib/printk.h>

const obj_deinit_func obj_deinit_tbl[TYPE_NR] = {
       [0 ... TYPE_NR - 1] = NULL,
       [TYPE_THREAD] = thread_deinit,
       [TYPE_IRQ] = irq_deinit,
};

void *obj_alloc(u64 type, u64 size)
{
	u64 total_size;
	struct object *object;

	// XXX: opaque is u64 so sizeof(*object) is 8-byte aligned.
	//      Thus the address of object-defined data is always 8-byte aligned.
	total_size = sizeof(*object) + size;
	object = kzalloc(total_size);
	/* TODO: errno ecoded in pointer */
	if (!object)
		return NULL;

	object->type = type;
	object->size = size;
	object->refcount = 0;
	init_list_head(&object->copies_head);
	lock_init(&object->copies_lock);

	return object->opaque;
}

/*
 * XXX: obj_free can be used in two conditions:
 * 1. In initialization of a cap (after obj_alloc and before cap_aloc) as a
 * fallback to obj_alloc
 * 2. To all slots which point to it. In this use case, the
 * caller must guarantee that the object is not freed (i.e., should hold a
 * reference to it).
 */
void obj_free(void *obj)
{
	struct object *object;
	struct object_slot *slot_iter = NULL, *slot_iter_tmp = NULL;
	int r;

	if (!obj)
		return;
	object = container_of(obj, struct object, opaque);

	/* fallback of obj_alloc */
	if (object->refcount == 0) {
		kfree(object);
		return;
	}

	/* free all copied slots */
	lock(&object->copies_lock);
	for_each_in_list_safe(slot_iter, slot_iter_tmp,
			      copies, &object->copies_head) {
		u64 iter_slot_id = slot_iter->slot_id;
		struct cap_group *iter_cap_group = slot_iter->cap_group;

		r = __cap_free(iter_cap_group, iter_slot_id, false, true);
		BUG_ON(r != 0);
	}
	unlock(&object->copies_lock);
}

int cap_alloc(struct cap_group *cap_group, void *obj, u64 rights)
{
	struct object *object;
	struct slot_table *slot_table;
	struct object_slot *slot;
	int r, slot_id;

	object = container_of(obj, struct object, opaque);
	slot_table = &cap_group->slot_table;

	write_lock(&slot_table->table_guard);
	slot_id = alloc_slot_id(cap_group);
	if (slot_id < 0) {
		r = -ENOMEM;
		goto out_unlock_table;
	}

	slot = kmalloc(sizeof(*slot));
	if (!slot) {
		r = -ENOMEM;
		goto out_free_slot_id;
	}
	slot->slot_id = slot_id;
	slot->cap_group = cap_group;
	slot->isvalid = true;
	slot->rights = rights;
	slot->object = object;
	list_add(&slot->copies, &object->copies_head);
	rwlock_init(&slot->slot_guard);

	BUG_ON(object->refcount != 0);
	object->refcount = 1;

	install_slot(cap_group, slot_id, slot);

	write_unlock(&slot_table->table_guard);
	return slot_id;
out_free_slot_id:
	free_slot_id(cap_group, slot_id);
out_unlock_table:
	write_unlock(&slot_table->table_guard);
	return r;
}

/*
 * General capability free function with table_guard and copies_lock
 * locked or unlocked.
 */
int __cap_free(struct cap_group *cap_group, int slot_id,
	       bool slot_table_locked, bool copies_list_locked)
{
	struct object_slot *slot;
	struct object *object;
	struct slot_table *slot_table;
	int r = 0;
	u64 old_refcount;
	obj_deinit_func func;

	/* Step-1: free the slot_id (i.e., the capability number) in the slot table */
	slot_table = &cap_group->slot_table;
	if (!slot_table_locked)
		write_lock(&slot_table->table_guard);
	slot = get_slot(cap_group, slot_id);
	if (!slot || slot->isvalid == false) {
		r = -ECAPBILITY;
		goto out_unlock_table;
	}

	free_slot_id(cap_group, slot_id);
	if (!slot_table_locked)
		write_unlock(&slot_table->table_guard);

	/* Step-2: remove the slot in the slot-list of the object and free the slot */
	object = slot->object;
	if (copies_list_locked) {
		list_del(&slot->copies);
	} else {
		lock(&object->copies_lock);
		list_del(&slot->copies);
		unlock(&object->copies_lock);
	}
	kfree(slot);

	/* Step-3: decrease the refcnt of the object and free it if necessary */
	old_refcount = atomic_fetch_sub_64(&object->refcount, 1);
	if (old_refcount == 1) {

#ifndef TEST_OBJECT
		func = obj_deinit_tbl[object->type];
#else
		/* For unit test only */
		extern obj_deinit_func obj_test_deinit_tbl[];
		func = obj_test_deinit_tbl[object->type];
#endif
		if (func)
			func(object->opaque);

		if (object->refcount == 0) {
			/* Assert: the copies list should be zero */
			BUG_ON(!list_empty(&object->copies_head));
			kfree(object);
		}
	}

	return r;

out_unlock_table:
	if (!slot_table_locked)
		write_unlock(&slot_table->table_guard);
	return r;
}

int cap_free(struct cap_group *cap_group, int slot_id)
{
	return __cap_free(cap_group, slot_id, false, false);
}

int cap_copy(struct cap_group *src_cap_group, struct cap_group *dest_cap_group,
	     int src_slot_id, bool new_rights_valid, u64 new_rights)
{
	struct object_slot *src_slot, *dest_slot;
	int r, dest_slot_id;
	struct rwlock *src_table_guard, *dest_table_guard;
	bool local_copy;

	local_copy = src_cap_group == dest_cap_group;
	src_table_guard = &src_cap_group->slot_table.table_guard;
	dest_table_guard = &dest_cap_group->slot_table.table_guard;
	if (local_copy) {
		write_lock(dest_table_guard);
	} else {
		/* avoid deadlock */
		while (true) {
			read_lock(src_table_guard);
			if (write_try_lock(dest_table_guard) == 0)
				break;
			read_unlock(src_table_guard);
		}
	}

	src_slot = get_slot(src_cap_group, src_slot_id);
	if (!src_slot || src_slot->isvalid == false) {
		r = -ECAPBILITY;
		goto out_unlock;
	}

	dest_slot_id = alloc_slot_id(dest_cap_group);
	if (dest_slot_id == -1) {
		r = -ENOMEM;
		goto out_unlock;
	}

	dest_slot = kmalloc(sizeof(*dest_slot));
	if (!dest_slot) {
		r = -ENOMEM;
		goto out_free_slot_id;
	}
	src_slot = get_slot(src_cap_group, src_slot_id);
	atomic_fetch_add_64(&src_slot->object->refcount, 1);

	dest_slot->slot_id = dest_slot_id;
	dest_slot->cap_group = dest_cap_group;
	dest_slot->isvalid = true;
	dest_slot->object = src_slot->object;
	dest_slot->rights = new_rights_valid ? new_rights : src_slot->rights;
	list_add(&dest_slot->copies, &src_slot->copies);
	rwlock_init(&dest_slot->slot_guard);

	install_slot(dest_cap_group, dest_slot_id, dest_slot);

	write_unlock(dest_table_guard);
	if (!local_copy)
		read_unlock(src_table_guard);
	return dest_slot_id;
out_free_slot_id:
	free_slot_id(dest_cap_group, dest_slot_id);
out_unlock:
	write_unlock(dest_table_guard);
	if (!local_copy)
		read_unlock(src_table_guard);
	return r;
}

/*
 * Copy capability within the same cap_group.
 * Returns new cap or error code.
 */
int cap_copy_local(struct cap_group *cap_group, int src_slot_id, u64 new_rights)
{
	return cap_copy(cap_group, cap_group, src_slot_id, true, new_rights);
}

int cap_move(struct cap_group *src_cap_group, struct cap_group *dest_cap_group,
	     int src_slot_id, bool new_rights_valid, u64 new_rights)
{
	int r;

	r = cap_copy(src_cap_group, dest_cap_group, src_slot_id,
		     new_rights_valid, new_rights);
	if (r < 0)
		return r;
	r = cap_free(src_cap_group, src_slot_id);
	BUG_ON(r); /* if copied successfully, free should not fail */

	return r;
}

int cap_revoke(struct cap_group *cap_group, int slot_id)
{
	void *obj;
	int r;

	/*
	 * Since obj_get requires to pass the cap type
	 * which is not available here, get_opaque is used instead.
	 */
	obj = get_opaque(cap_group, slot_id, false, 0);

	if (!obj) {
		r =  -ECAPBILITY;
		goto out_fail;
	}

	obj_free(obj);

	/* get_opaque will also add the reference cnt */
	obj_put(obj);

	return 0;

out_fail:
	return r;
}

int sys_cap_copy_to(u64 dest_cap_group_cap, u64 src_slot_id)
{
	struct cap_group *dest_cap_group;
	int r;

	dest_cap_group = obj_get(current_cap_group, dest_cap_group_cap,
				 TYPE_CAP_GROUP);
	if (!dest_cap_group)
		return -ECAPBILITY;
	r = cap_copy(current_cap_group, dest_cap_group, src_slot_id, 0, 0);
	obj_put(dest_cap_group);
	return r;
}

int sys_cap_copy_from(u64 src_cap_group_cap, u64 src_slot_id)
{
	struct cap_group *src_cap_group;
	int r;

	src_cap_group = obj_get(current_cap_group, src_cap_group_cap,
				TYPE_CAP_GROUP);
	if (!src_cap_group)
		return -ECAPBILITY;
	r = cap_copy(src_cap_group, current_cap_group, src_slot_id, 0, 0);
	obj_put(src_cap_group);
	return r;
}

#ifndef FBINFER
int sys_transfer_caps(u64 dest_group_cap, u64 src_caps_buf, int nr_caps,
		     u64 dst_caps_buf)
{
	struct cap_group *dest_cap_group;
	int i;
	int *src_caps;
	int *dst_caps;
	size_t size;

	dest_cap_group = obj_get(current_cap_group, dest_group_cap,
				 TYPE_CAP_GROUP);
	if (!dest_cap_group)
		return -ECAPBILITY;

	size = sizeof(int) * nr_caps;
	src_caps = kmalloc(size);
	dst_caps = kmalloc(size);

	/* get args from user buffer */
	copy_from_user((void *)src_caps, (void *)src_caps_buf, size);

	for (i = 0; i < nr_caps; ++i) {
		// TODO: check error
		dst_caps[i] = cap_copy(current_cap_group, dest_cap_group,
			     src_caps[i], 0, 0);
	}

	/* write results to user buffer */
	copy_to_user((void *)dst_caps_buf, (void *)dst_caps, size);

	obj_put(dest_cap_group);
	return 0;
}
#endif

int sys_cap_move(u64 dest_cap_group_cap, u64 src_slot_id)
{
	struct cap_group *dest_cap_group;
	int r;

	dest_cap_group = obj_get(current_cap_group, dest_cap_group_cap,
				 TYPE_CAP_GROUP);
	if (!dest_cap_group)
		return -ECAPBILITY;
	r = cap_move(current_cap_group, dest_cap_group, src_slot_id, 0, 0);
	obj_put(dest_cap_group);
	return r;
}

// for debug
int sys_get_all_caps(u64 cap_group_cap)
{
	struct cap_group *cap_group;
	struct slot_table *slot_table;
	int i;

	cap_group = obj_get(current_cap_group, cap_group_cap,
			    TYPE_CAP_GROUP);
	if (!cap_group)
		return -ECAPBILITY;
	printk("thread %p cap:\n", current_thread);

	slot_table = &cap_group->slot_table;
	for (i = 0; i < slot_table->slots_size; i++) {
		struct object_slot *slot = get_slot(cap_group, i);
		if (!slot)
			continue;
		BUG_ON(slot->isvalid != true);
		printk("slot_id:%d type:%d\n", i,
		       slot_table->slots[i]->object->type);
	}

	obj_put(cap_group);
	return 0;
}
