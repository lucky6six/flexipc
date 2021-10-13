#include <object/object.h>
#include <object/thread.h>
#include <mm/vmspace.h>
#include <mm/uaccess.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <common/lock.h>
#include <arch/mmu.h>

#include "mmap.h"

int sys_create_device_pmo(u64 paddr, u64 size)
{
	int cap, r;
	struct pmobject *pmo;

	BUG_ON(size == 0);
	pmo = obj_alloc(TYPE_PMO, sizeof(*pmo));
	if (!pmo) {
		r = -ENOMEM;
		goto out_fail;
	}
	pmo_init(pmo, PMO_DEVICE, size, paddr);
	cap = cap_alloc(current_cap_group, pmo, 0);
	if (cap < 0) {
		r = cap;
		goto out_free_obj;
	}

	return cap;
out_free_obj:
	obj_free(pmo);
out_fail:
	return r;
}

int sys_create_pmo(u64 size, u64 type)
{
	int cap, r;
	struct pmobject *pmo;

	BUG_ON(size == 0);
	pmo = obj_alloc(TYPE_PMO, sizeof(*pmo));
	if (!pmo) {
		r = -ENOMEM;
		goto out_fail;
	}
	pmo_init(pmo, type, size, 0);
	cap = cap_alloc(current_cap_group, pmo, 0);
	if (cap < 0) {
		r = cap;
		goto out_free_obj;
	}

	return cap;
out_free_obj:
	obj_free(pmo);
out_fail:
	return r;
}

struct pmo_request {
	/* args */
	u64 size;
	u64 type;
	/* return value */
	u64 ret_cap;
};

#define MAX_CNT 32

int sys_create_pmos(u64 user_buf, u64 cnt)
{
	u64 size;
	struct pmo_request *requests;
	int i;
	int cap;

	/* in case of integer overflow */
	if (cnt > MAX_CNT) {
		kwarn("create too many pmos for one time (max: %d)\n", MAX_CNT);
		return -EINVAL;
	}

	/* TODO: can we directly read/write user buffers */
	size = sizeof(*requests) * cnt;
	requests = (struct pmo_request *)kmalloc(size);
	if (requests == NULL) {
		kwarn("cannot allocate more memory\n");
		return -EAGAIN;
	}
	copy_from_user((char *)requests, (char *)user_buf, size);

	for (i = 0; i < cnt; ++i) {
		cap = sys_create_pmo(requests[i].size, requests[i].type);
		/*
		 * TODO: what if some errors occur (i.e., create part of pmos).
		 * levave it to user space for now.
		 */
		requests[i].ret_cap = cap;
	}

	/* return pmo_caps */
	copy_to_user((char *)user_buf, (char *)requests, size);

	/* free temporary buffer */
	kfree(requests);

	return 0;
}

#define WRITE 0
#define READ  1
static int read_write_pmo(u64 pmo_cap, u64 offset, u64 user_buf,
			  u64 size, u64 op_type)
{
	struct pmobject *pmo;
	pmo_type_t pmo_type;
	vaddr_t kva;
	int r;

	r = 0;
	/* Function caller should have the pmo_cap */
	pmo = obj_get(current_cap_group, pmo_cap, TYPE_PMO);
	if (!pmo) {
		r = -ECAPBILITY;
		goto out_fail;
	}

	/* Only PMO_DATA or PMO_ANONYM is allowed with this inferface. */
	pmo_type = pmo->type;
	if ((pmo_type != PMO_DATA) && (pmo_type != PMO_ANONYM)) {
		r = -EINVAL;
		goto out_obj_put;
	}

	/* Range check */
	if (offset + size < offset || offset + size > pmo->size) {
		r = -EINVAL;
		goto out_obj_put;
	}

	if (pmo_type == PMO_DATA) {
		kva = phys_to_virt(pmo->start) + offset;
	} else {
		/* PMO_ANONYM */
		u64 index;
		u64 pa;

		index = ROUND_DOWN(offset, PAGE_SIZE) / PAGE_SIZE;
		pa = get_page_from_pmo(pmo, index);
		if (pa == 0) {
			/* Allocate a physical page for the anonymous pmo
			 * like a page fault happens.
			 */
			kva = (vaddr_t)get_pages(0);
			BUG_ON(kva == 0);

			pa = virt_to_phys((void *)kva);
			memset((void *)kva, 0, PAGE_SIZE);
			commit_page_to_pmo(pmo, index, pa);

			/* No need to map the physical page in the page table
			 * of current process because it uses write/read_pmo
			 * which means it does not need the mappings.
			 */
		}
		else {
			kva = phys_to_virt(pa);
		}
	}

	if (op_type == WRITE) {
		r = copy_from_user((char *)kva, (char *)user_buf, size);
	}
	else if (op_type == READ)
		r = copy_to_user((char *)user_buf, (char *)kva, size);
	else
		BUG("Only PMO read/write is possible.\n");

out_obj_put:
	obj_put(pmo);
out_fail:
	return r;
}

/*
 * A process can send a PMO (with msgs) to others.
 * It can write the msgs without mapping the PMO with this function.
 */
int sys_write_pmo(u64 pmo_cap, u64 offset, u64 user_ptr, u64 len)
{
	return read_write_pmo(pmo_cap, offset, user_ptr, len, WRITE);
}

int sys_read_pmo(u64 pmo_cap, u64 offset, u64 user_ptr, u64 len)
{
	return read_write_pmo(pmo_cap, offset, user_ptr, len, READ);
}

/**
 * Given a pmo_cap, return its corresponding start physical address.
 */
int sys_get_pmo_paddr(u64 pmo_cap, u64 user_buf)
{
	struct pmobject *pmo;
	int r = 0;

	/* Caller should have the pmo_cap */
	pmo = obj_get(current_cap_group, pmo_cap, TYPE_PMO);
	if (!pmo) {
		r = -ECAPBILITY;
		goto out_fail;
	}

	/* Only allow to get the address of PMO_DATA for now */
	if (pmo->type != PMO_DATA) {
		r = -EINVAL;
		goto out_obj_put;
	}

	copy_to_user((char *)user_buf, (char *)&pmo->start, sizeof(u64));

out_obj_put:
	obj_put(pmo);
out_fail:
	return r;
}

/*
 * Given a virtual address, return its corresponding physical address.
 * Notice: the virtual address should be page-backed, thus pre-fault could be
 * conducted before using this syscall.
 */
int sys_get_phys_addr(u64 va , u64 *pa_buf)
{
	struct vmspace *vmspace = current_thread->vmspace;
	paddr_t pa;
	int ret;

	lock(&vmspace->pgtbl_lock);
	extern int query_in_pgtbl(vaddr_t *, vaddr_t, paddr_t *, void **);
	ret = query_in_pgtbl(vmspace->pgtbl, va, &pa, NULL);
	unlock(&vmspace->pgtbl_lock);

	if (ret < 0)
		return ret;

	copy_to_user((char *)pa_buf, (char *)&pa, sizeof(u64));

	return 0;
}


int trans_uva_to_kva(u64 user_va, u64 *kernel_va)
{
	struct vmspace *vmspace = current_thread->vmspace;
	paddr_t pa;
	int ret;

	lock(&vmspace->pgtbl_lock);
	extern int query_in_pgtbl(vaddr_t *, vaddr_t, paddr_t *, void **);
	ret = query_in_pgtbl(vmspace->pgtbl, user_va, &pa, NULL);
	unlock(&vmspace->pgtbl_lock);

	if (ret < 0)
		return ret;

	*kernel_va = phys_to_virt(pa);
	return 0;
}

/*
 * A process can not only map a PMO into its private address space,
 * but also can map a PMO to some others (e.g., load code for others).
 */
int sys_map_pmo(u64 target_cap_group_cap, u64 pmo_cap, u64 addr, u64 perm)
{
	struct vmspace *vmspace;
	struct pmobject *pmo;
	struct cap_group *target_cap_group;
	int r;

	pmo = obj_get(current_cap_group, pmo_cap, TYPE_PMO);
	if (!pmo) {
		r = -ECAPBILITY;
		goto out_fail;
	}

	/* map the pmo to the target cap_group */
	target_cap_group = obj_get(current_cap_group, target_cap_group_cap,
				   TYPE_CAP_GROUP);
	if (!target_cap_group) {
		r = -ECAPBILITY;
		goto out_obj_put_pmo;
	}
	vmspace = obj_get(target_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
	BUG_ON(vmspace == NULL);

	// TODO (question): is it required to restrict pmo mapping?
	// TODO: check wheter perm is legal
	// TODO: check addr validation
	r = vmspace_map_range(vmspace, addr, pmo->size, perm, pmo);
	if (r != 0) {
		r = -EPERM;
		goto out_obj_put_vmspace;
	}

	/*
	 * when a process maps a pmo to others,
	 * this func returns the new_cap in the target process.
	 */
	if (target_cap_group != current_cap_group)
		/* if using cap_move, we need to consider remove the mappings */
		r = cap_copy(current_cap_group, target_cap_group,
			       pmo_cap, 0, 0);
	else
		r = 0;

out_obj_put_vmspace:
	obj_put(vmspace);
	obj_put(target_cap_group);
out_obj_put_pmo:
	obj_put(pmo);
out_fail:
	return r;
}

/* Example usage: Used in ipc/connection.c for mapping ipc_shm */
int map_pmo_in_current_cap_group(u64 pmo_cap, u64 addr, u64 perm)
{
	struct vmspace *vmspace;
	struct pmobject *pmo;
	int r;

	pmo = obj_get(current_cap_group, pmo_cap, TYPE_PMO);
	if (!pmo) {
		kinfo("map fails: invalid pmo (cap is %lu)\n", pmo_cap);
		r = -ECAPBILITY;
		goto out_fail;
	}

	vmspace = obj_get(current_cap_group, VMSPACE_OBJ_ID,
			  TYPE_VMSPACE);
	BUG_ON(vmspace == NULL);
	r = vmspace_map_range(vmspace, addr, pmo->size, perm, pmo);
	if (r != 0) {
		kinfo("%s failed: addr 0x%lx, pmo->size 0x%lx\n",
		      __func__, addr, pmo->size);
		r = -EPERM;
		goto out_obj_put_vmspace;
	}

out_obj_put_vmspace:
	obj_put(vmspace);
	obj_put(pmo);
out_fail:
	return r;
}


struct pmo_map_request {
	/* args */
	u64 pmo_cap;
	u64 addr;
	u64 perm;

	/* return caps or return value */
	u64 ret;
};

int sys_map_pmos(u64 target_cap_group_cap, u64 user_buf, u64 cnt)
{
	u64 size;
	struct pmo_map_request *requests;
	int i;
	int ret;

	/* in case of integer overflow */
	if (cnt > MAX_CNT) {
		kwarn("create too many pmos for one time (max: %d)\n", MAX_CNT);
		return -EINVAL;
	}

	/* TODO: can we directly read/write user buffers */
	size = sizeof(*requests) * cnt;
	requests = (struct pmo_map_request *)kmalloc(size);
	if (requests == NULL) {
		kwarn("cannot allocate more memory\n");
		return -EAGAIN;
	}
	copy_from_user((char *)requests, (char *)user_buf, size);

	for (i = 0; i < cnt; ++i) {
		/*
		 * if target_cap_group is not current_cap_group,
		 * ret is cap on success.
		 */
		ret = sys_map_pmo(target_cap_group_cap,
				  requests[i].pmo_cap,
				  requests[i].addr,
				  requests[i].perm);
		/*
		 * TODO: what if some errors occur (i.e., create part of pmos).
		 * levave it to user space for now.
		 */
		requests[i].ret = ret;
	}

	copy_to_user((char *)user_buf, (char *)requests, size);

	kfree(requests);
	return 0;
}

/* TODO: add sys_unmap_pmos */
int sys_unmap_pmo(u64 target_cap_group_cap, u64 pmo_cap, u64 addr)
{
	struct vmspace *vmspace;
	struct pmobject *pmo;
	struct cap_group *target_cap_group;
	int ret;

	/* caller should have the pmo_cap */
	pmo = obj_get(current_cap_group, pmo_cap, TYPE_PMO);
	if (!pmo)
		return -EPERM;

	/* map the pmo to the target cap_group */
	target_cap_group = obj_get(current_cap_group, target_cap_group_cap,
				   TYPE_CAP_GROUP);
	if (!target_cap_group) {
		ret = -EPERM;
		goto fail1;
	}

	vmspace = obj_get(target_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
	if (!vmspace) {
		ret = -EPERM;
		goto fail2;
	}

	// TODO (question): is it required to restrict pmo mapping?
	ret = vmspace_unmap_range(vmspace, addr, pmo->size);
	if (ret != 0)
		ret = -EPERM;

	obj_put(vmspace);
fail2:
	obj_put(target_cap_group);
fail1:
	obj_put(pmo);

	return ret;
}


/*
 * User process heap start: 0x600000000000 (i.e., HEAP_START)
 *
 * defined in mm/vmregion.c
 */

/*
 * TODO (tmac): we should modify LibC malloc as follows:
 * instead of invoking brk(0) at first, it should create the heap pmo by itself.
 */
u64 sys_handle_brk(u64 addr) {
	struct vmspace *vmspace;
	struct pmobject *pmo;
	struct vmregion *vmr;
	size_t len;
	u64 retval;
	int ret;


	vmspace = obj_get(current_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
	lock(&vmspace->heap_lock);
	if (addr == 0) {
		/*
		 * Assumation: user/libc invokes brk(0) first.
		 *
		 * return the current heap address
		 */
		retval = vmspace->user_current_heap;

		/* create the heap pmo for the user process */
		pmo = obj_alloc(TYPE_PMO, sizeof(*pmo));
		if (!pmo) {
			kinfo("Fail: cannot create the initial heap pmo.\n");
			BUG_ON(1);
		}
		/* set the heap pmo type as PMO_ANONYM */
		len = 0;
		pmo_init(pmo, PMO_ANONYM, len, 0);
		ret = cap_alloc(current_cap_group, pmo, 0);
		if (ret < 0) {
			kinfo("Fail: cannot create cap for the initial heap pmo.\n");
			BUG_ON(1);
		}

		/* setup the vmr for the heap region */
		vmspace->heap_vmr = init_heap_vmr(vmspace, retval, pmo);
	}
	else {
		vmr = vmspace->heap_vmr;
		/* enlarge the heap vmr and pmo */
		if (addr >= (vmr->start + vmr->size)) {
			/* add length */
			len = addr - (vmr->start + vmr->size);
			vmr->size += len;
			vmr->pmo->size += len;
		}
		else {
			kinfo("Buggy: why shrinking the heap?\n");
			BUG_ON(1);
		}

		retval = addr;
	}


	/*
	 * return origin heap addr on failure;
	 * return new heap addr on success.
	 */
	unlock(&vmspace->heap_lock);
	obj_put(vmspace);
	return retval;
}

/* A process mmap region start:  MMAP_START (defined in mm/vmregion.c) */
static vmr_prop_t get_vmr_prot(int prot)
{
	vmr_prop_t ret;

	ret = 0;
	if (prot & PROT_READ)
		ret |= VMR_READ;
	if (prot & PROT_WRITE)
		ret |= VMR_WRITE;
	if (prot & PROT_EXEC)
		ret |= VMR_EXEC;

	return ret;
}

extern u64 vmspace_mmap_with_pmo(struct vmspace *vmspace, struct pmobject *pmo,
				 size_t len, vmr_prop_t perm);
u64 sys_handle_mmap(u64 addr, size_t length, int prot, int flags,
		    int fd, u64 offset)
{
	struct vmspace *vmspace;
	struct pmobject *pmo;
	vmr_prop_t vmr_prot;
	u64 map_addr;
	int new_pmo_cap;

	/* Currently, mmap must takes @fd with -1 (i.e., anonymous mapping) */
	if (fd != -1) {
		kwarn("%s: mmap only supports anonymous mapping with fd -1, but arg fd is %d\n",
		      __func__, fd);
		BUG("");
		goto err_exit;
	}

	/* Check @prot */
	if (prot & PROT_CHECK_MASK) {
		kwarn("%s: mmap cannot support PROT: %d\n", __func__, prot);
		goto err_exit;
	}

	/* Check @flags */
	if (flags != (MAP_ANONYMOUS | MAP_PRIVATE)) {
		kwarn("%s: mmap only supports anonymous and private mapping\n",
		      __func__);
		goto err_exit;
	}

	/* Add one anonymous mapping, i.e., the cap in new_pmo_cap */

	/* Round up @length */
	if (length % PAGE_SIZE) {
		// kwarn("%s: mmap length should align to PAGE_SIZE\n", __func__);
		length = ROUND_UP(length, PAGE_SIZE);
	}

	/* Create the pmo for the mmap area */
	pmo = obj_alloc(TYPE_PMO, sizeof(*pmo));
	if (!pmo) {
		kinfo("Fail: cannot create the initial heap pmo.\n");
		BUG_ON(1);
	}
	/* Set the pmo type as PMO_ANONYM */
	pmo_init(pmo, PMO_ANONYM, length, 0);

	/* Create the cap for this pmo */
	new_pmo_cap = cap_alloc(current_cap_group, pmo, 0);
	if (new_pmo_cap < 0) {
		kinfo("Fail: cannot create cap for the new pmo\n");
		BUG_ON(1);
	}

	/* Change the vmspace */
	vmspace = obj_get(current_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
	vmr_prot = get_vmr_prot(prot);
	map_addr = vmspace_mmap_with_pmo(vmspace, pmo, length, vmr_prot);
	obj_put(vmspace);

	kdebug("mmap done: map_addr is %p\n", map_addr);
	return map_addr;

err_exit:
	map_addr = (u64)(-1L);
	return map_addr;
}

/*
 * According to the POSIX specification:
 *
 * The munmap() function shall remove any mappings for those entire pages
 * containing any part of the address space of the process starting at addr and
 * continuing for len bytes.
 *
 * The behavior of this function is unspecified if the mapping was not
 * established by a call to mmap().
 *
 * The address addr  must be a multiple of the page size
 * (but length need not be).
 * All pages containing a part of the indicated range are unmapped,
 * and subsequent references to these pages will generate SIGSEGV.
 * It is not an error if the indicated range does not contain any mapped pages.
 *
 */
extern int vmspace_munmap_with_addr(struct vmspace* vmspace,
				    vaddr_t addr, size_t len);
int sys_handle_munmap(u64 addr, size_t length)
{
	struct vmspace *vmspace;
	int ret;

	/* What if the len is not aligned */
	if ((addr % PAGE_SIZE) || (length % PAGE_SIZE)) {
		return -EINVAL;
	}

	vmspace = obj_get(current_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
	ret = vmspace_munmap_with_addr(vmspace, (vaddr_t)addr, length);
	obj_put(vmspace);

	return ret;
}

/* vmr_prot must contains target_prot */
static int valid_prot(vmr_prop_t vmr_prot, vmr_prop_t target_prot)
{
	if ((target_prot & PROT_READ) && !(vmr_prot & PROT_READ))
		return -1;

	if ((target_prot & PROT_WRITE) && !(vmr_prot & PROT_WRITE))
		return -1;

	if ((target_prot & PROT_EXEC) && !(vmr_prot & PROT_EXEC))
		return -1;

	return 0;
}

extern int mprotect_in_pgtbl(struct vmspace *, vaddr_t, size_t, vmr_prop_t);

int sys_handle_mprotect(u64 addr, size_t length, int prot)
{
	vmr_prop_t target_prot;
	struct vmspace *vmspace;
	struct vmregion *vmr;
	s64 remaining;
	u64 va;
	int ret;

	if ((addr % PAGE_SIZE) || (length % PAGE_SIZE)) {
		return -EINVAL;
	}

	if (length == 0) return 0;

	target_prot = get_vmr_prot(prot);
	vmspace = obj_get(current_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);

	lock(&vmspace->vmspace_lock);
	/*
	 * Validate the VM range [addr, addr + lenght]
	 * - the range is totally mapped
	 * - the range has more permission than prot
	 */
	va = addr;
	remaining = length;
	while (remaining > 0) {
		vmr = find_vmr_for_va(vmspace, va);

		if (!vmr) {
			ret = -EACCES;
			goto out;
		}

		if (valid_prot(vmr->perm, target_prot)) {
			ret = -EACCES;
			goto out;
		}

		if (remaining < vmr->size) {

			static int warn = 1;

			if (warn == 1) {
				kwarn("func: %s, ignore a mprotect since no supporting for "
				      "splitting vmr now.\n", __func__);
				warn = 0;
			}

			// TODO: mprotect a part of a vmr
			// no support splitting the region now
			// ret = -EINVAL;
			// goto out;

			// TODO: do nothing
			// passing valid_prot means permission reducing
			// just igonre this syscall now

			obj_put(vmspace);
			unlock(&vmspace->vmspace_lock);

			ret = 0;
			return ret;
		}

		remaining -= vmr->size;
		va += vmr->size;
	}

	BUG_ON(remaining != 0);

#ifndef FBINFER
	/* Change the prot in each vmr */
	va = addr;
	remaining = length;

	while (remaining > 0) {
		vmr = find_vmr_for_va(vmspace, va);
		vmr->perm = target_prot;

		remaining -= vmr->size;
		va += vmr->size;
	}
#endif

	/* Modify the existing mappings in pgtbl */
	lock(&vmspace->pgtbl_lock);
	mprotect_in_pgtbl(vmspace, addr, length, target_prot);
	unlock(&vmspace->pgtbl_lock);
	ret = 0;

out:
	obj_put(vmspace);
	unlock(&vmspace->vmspace_lock);

	return ret;
}


/*
 * For POSIX-compatability, we also need to support shmxxx interfaces.
 * We now implement them here.
 *
 * TODO: Use a Namespace server and modify libc/src/ipc
 * for handling shmxxx interfaces.
 */

struct shm_record {
	int key;
	int shmid;
	size_t size;

	struct list_head node;

	/* PMO_SHM */
	struct pmobject *pmo;
};

//struct list_head global_shm_info = {&global_shm_info, &global_shm_info};
struct list_head global_shm_info;
int global_shm_id;
/* Protect the global_shm_info list and the gobal_shm_id */
struct lock shm_info_lock;

void init_global_shm_namespace(void)
{
	init_list_head(&global_shm_info);
	global_shm_id = 0;
	lock_init(&shm_info_lock);
}

#define IPC_CREAT 01000
#define IPC_EXCL  02000

#define SHM_RDONLY 010000
#define SHM_RND    020000
#define SHM_REMAP  040000
#define SHM_EXEC   0100000

int sys_shmget(int key, size_t size, int flag)
{
	int shmid;
	struct shm_record *record;
	int exist;
	struct pmobject *pmo;

	exist = 0;

	/* TODO: allow more flags? */
	BUG_ON(!(flag & IPC_CREAT));

	lock(&shm_info_lock);

	for_each_in_list(record, struct shm_record, node, &global_shm_info) {
		if (record->key == key) {
			exist = 1;
			break;
		}
	}

	/* Already exist */
	if ((exist == 1) && (flag & IPC_EXCL)) {
		shmid = -EEXIST;
		goto out;
	}

	/* Return the exist shmid */
	if (exist == 1) {
		shmid = record->shmid;
		goto out;
	}

	size = ROUND_UP(size, PAGE_SIZE);

	/* Allocate a new shmid */
	record = (struct shm_record *)kmalloc(sizeof(*record));
	record->key = key;
	record->shmid = ++global_shm_id; /* Starts from 1 */
	record->size = size;
	/* Allocate a PMO_SHM for the new shmid */
	pmo = obj_alloc(TYPE_PMO, sizeof(*pmo));
	if (!pmo) {
		shmid = -ENOMEM;
		goto out;
	}
	pmo_init(pmo, PMO_SHM, size, 0);
	record->pmo = pmo;

	/* Record the shmid */
	list_add(&(record->node), &global_shm_info);

	shmid = record->shmid;

out:
	unlock(&shm_info_lock);
	return shmid;
}

u64 sys_shmat(int shmid, u64 shmaddr, int shmflg)
{
	struct shm_record *record;
	int exist;
	vmr_prop_t perm;
	struct vmspace *vmspace;

	exist = 0;

	lock(&shm_info_lock);

	for_each_in_list(record, struct shm_record, node, &global_shm_info) {
		if (record->shmid == shmid) {
			exist = 1;
			break;
		}
	}


	if (exist != 1) {
		shmaddr = -EINVAL;
		goto out;
	}

	if ((shmaddr != 0) && (shmflg & SHM_RND)) {
		kwarn("No support for specific shmaddr now.\n");
		shmaddr = -EINVAL;
		goto out;
	}

	if (shmflg & SHM_REMAP) {
		kwarn("No support for SHM_REMAP now.\n");
		shmaddr = -EINVAL;
		goto out;
	}


	/* Set the permission */
	if (shmflg & SHM_RDONLY) {
		perm = VMR_READ;
	} else if (shmflg & SHM_EXEC) {
		perm = VMR_READ | VMR_WRITE | VMR_EXEC;
	} else {
		perm = VMR_READ | VMR_WRITE;
	}

	vmspace = obj_get(current_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
	/* Use the interface " vmspace_mmap_with_pmo" */
	shmaddr = vmspace_mmap_with_pmo(vmspace, record->pmo,
					record->size, perm);
	obj_put(vmspace);

out:
	/*
	 * Unlock the shm_info_lock after mapping the record
	 * in case of free by someone.
	 */
	unlock(&shm_info_lock);
	return shmaddr;
}

extern int vmspace_unmap_shm_vmr(struct vmspace *, vaddr_t);

int sys_shmdt(u64 shmaddr)
{
	struct vmspace *vmspace;
	int ret;

	/*
	 * TODO: only a temporary impl.
	 * Now just unmap the whole corresponding vmr of shmaddr.
	 */

	vmspace = obj_get(current_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
	ret = vmspace_unmap_shm_vmr(vmspace, shmaddr);
	obj_put(vmspace);

	return ret;
}

int sys_shmctl(int shmid, int cmd, u64 buf)
{
	kwarn("No support for shmctl now.\n");
	return 0;
}



/* System call: mmap with file */
u64 sys_handle_fmap(u64 addr, size_t length, int prot, int flags,
		    int translated_aarray_cap, u64 offset)
{
	struct vmspace *vmspace;
	struct pmobject *aarray_pmo;
	vmr_prop_t vmr_prot;
	u64 map_addr;
	int err;

	/* The translated aaray cap is mandatory. */
	if (translated_aarray_cap == 0) {
		kwarn("%s: Invalid translated aarray cap\n", __func__);
		err = -EINVAL;
		goto err_exit;
	}

	if (prot & PROT_CHECK_MASK) {
		kwarn("%s: cannot support PROT: 0x%x\n", __func__, prot);
		err = -EINVAL;
		goto err_exit;
	}

	if (flags & MAP_ANONYMOUS) {
		kwarn("%s: You cannot use anonymous for fmap\n", __func__);
		err = -EINVAL;
		goto err_exit;
	}

	if (flags != MAP_SHARED) {
		kwarn("%s: fmap only supports MAP_SHARED, 0x%x.\n"
		      "But we allow other values for fast dev."
		      " Shall we make this CoW?\n",
		      __func__, flags);
	}

	if (length % PAGE_SIZE) {
		kwarn("%s: fmap length should align to PAGE_SIZE\n", __func__);
		err = -EINVAL;
		goto err_exit;

	}

	/* Get the translated aarray pmo object. */
	aarray_pmo = obj_get(current_cap_group, translated_aarray_cap,
			     TYPE_PMO);
	if (!aarray_pmo) {
		kwarn("fail to get the translated aarray pmo.\n");
		BUG_ON(1);
	}

	/* Change the vmspace */
	vmspace = obj_get(current_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
	vmr_prot = get_vmr_prot(prot);
	map_addr = vmspace_mmap_with_pmo(vmspace, aarray_pmo, length, vmr_prot);
	obj_put(vmspace);


	return map_addr;

err_exit:
	return (u64)err;
}

/**
 * aarray ::= address array
 * An address array is an array that contains virtual addresses.
 * This syscall takes an aarray as the input and translate all addresses in the
 * aarray to physical addresses.
 * These physical addresses are stored in the given memory indicated by the pmo
 * parameter. If the pmo is not provided, a new pmo (and its memory) is created
 * and returned.
 *
 * With the offset and count parameters, this syscall only translate a part of
 * the whole aarray. Note that the output is also updated in the given offset of
 * the pmo.
 *
 * The aarray_size is used to check offset and count and create new pmo.
 *
 * The aarray_size is in bytes;
 * The offset and count are in vaddr_ts.
 */
int sys_translate_aarray(vaddr_t *aarray, size_t aarray_size,
			 off_t offset, size_t count, int *pmo_p)
{
	struct vmspace *vmspace = current_thread->vmspace;
	int err = 0;
	int pmo_cap;
	struct pmobject *pmo;
	int i;

	if (count == 0)
		return 0;

	/* Check offset and + count < end of aarray */
	if ((offset + count) * sizeof(vaddr_t) > aarray_size)
		return -EINVAL;

	copy_from_user((char *)&pmo_cap, (char *)pmo_p, sizeof(pmo_cap));

	/* FIXME(MK): Should this be -1? */
	if (pmo_cap == 0) {
		/* Create an empty pmo. */
		pmo_cap = sys_create_pmo(aarray_size, PMO_FILE);
		BUG_ON(!pmo_cap);
	}
	pmo = obj_get(current_cap_group, pmo_cap, TYPE_PMO);
	BUG_ON(!pmo);

	for (i = 0; i < count; ++i) {
		vaddr_t va;
		paddr_t pa;

		copy_from_user((char *)&va,
			       (char *)((vaddr_t *)aarray + (offset + i)),
			       sizeof(va));

		if (va & (PAGE_SIZE - 1)) {
			kwarn("%s: The va (%p) should align to PAGE_SIZE\n",
			      __func__, va);
		}

		lock(&vmspace->pgtbl_lock);
		extern int query_in_pgtbl(vaddr_t *, vaddr_t, paddr_t *,
					  void **);
		err = query_in_pgtbl(vmspace->pgtbl, va, &pa, NULL);
		unlock(&vmspace->pgtbl_lock);

		/* This should never fail. */
		BUG_ON(err != 0);

		/* Add paddr to the pmo. */
		commit_page_to_pmo(pmo, offset + i, pa);

	}

	copy_to_user((char *)pmo_p, (char *)&pmo_cap, sizeof(pmo_cap));

	/* FIXME(MK): Could we put pmo earilier? */
	obj_put(pmo);

	return 0;
}
