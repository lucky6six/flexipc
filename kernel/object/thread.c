#include <common/kprint.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/util.h>
#include <lib/elf.h>
#include <mm/kmalloc.h>
#include <mm/mm.h>
#include <mm/uaccess.h>
#include <object/thread.h>
#include <sched/context.h>
#include <arch/machine/registers.h>
#include <arch/machine/smp.h>
#include <arch/time.h>
#include <lib/cpio.h>
#include <irq/ipi.h>

#include "thread_env.h"

extern const char binary_init_cpio_bin_start;

/*
 * extern functions
 */
int sys_map_pmo(u64 target_cap_group_cap, u64 pmo_cap, u64 addr, u64 perm);

/*
 * local functions
 */
#ifdef CHCORE
static int thread_init(struct thread *thread, struct cap_group *cap_group,
#else /* For unit test */
int thread_init(struct thread *thread, struct cap_group *cap_group,
#endif
		u64 stack, u64 pc, u32 prio, u32 type, s32 aff)
{
	/* XXX: no need to get/put */
	thread->cap_group = obj_get(cap_group, CAP_GROUP_OBJ_ID,
				    TYPE_CAP_GROUP);
	thread->vmspace = obj_get(cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
	obj_put(thread->cap_group);
	obj_put(thread->vmspace);
	/* Thread context is used as the kernel stack for that thread */
	thread->thread_ctx = create_thread_ctx();
	if (!thread->thread_ctx)
		return -ENOMEM;
	init_thread_ctx(thread, stack, pc, prio, type, aff % PLAT_CPU_NUM);

	/*
	 * Field prev_thread records the previous thread runs
	 * just before this thread. Obviously, it is NULL at the beginning.
	 */
	thread->prev_thread = NULL;

	/* The ipc_config will be allocated on demand */
	thread->general_ipc_config = NULL;


	thread->sleep_state.cb = NULL;

	return 0;
}

// TODO: modify this

/*
 * Thread can be freed in two ways:
 * 1. Free through cap. The thread can be in arbitary state except for TS_EXIT
 *    and TS_EXITING. If the state is TS_RUNNING, let the scheduler free it
 *    later as the context of thread is used when entering kernel.
 * 2. Free through sys_exit. The thread is TS_EXIT and can be directly freed.
 */
void thread_deinit(void *thread_ptr)
{
	struct thread *thread;
	struct cap_group *cap_group;
	// bool exit_cap_group = false;

	thread = thread_ptr;
	/* FIXME: data race: 1. TS_RUNNING thread can enter kernel thus ipi is
	 * sent but received after another thread is scheduled to userspace.
	 * 2. TS_READY thread can be dequeued and set TS_RUNNING. */
	switch (thread->thread_ctx->state) {
	case TS_RUNNING:

		/* This should not happen */
		BUG_ON(1);

#if 0
		struct object *object;
		/* XXX: defer the free of thread to when it is scheduled out */
		object = container_of(thread, struct object, opaque);
		object->refcount = 1;
		thread->thread_ctx->state = TS_EXITING;

		/* TODO: IPI is unsupported on x86 */
		/* FIXME: when the thread in on current CPU (most cases),
		 * no IPI is needed.
		 */
		ipi_reschedule(thread->thread_ctx->cpuid);
#endif
		break;
	case TS_READY:
		sched_dequeue(thread);
		/* fall through */
	default:
		cap_group = thread->cap_group;
		lock(&cap_group->threads_lock);
		list_del(&thread->node);
		// if (list_empty(&cap_group->thread_list))
			//exit_cap_group = true;

		unlock(&cap_group->threads_lock);
		destroy_thread_ctx(thread);
	}

	// TODO: remove this
	//if (exit_cap_group)
		//cap_group_exit(cap_group);
}

void handle_pending_exit()
{
	struct thread *thread = current_thread;
	if (unlikely(thread && thread->thread_ctx->state == TS_EXITING)) {
		thread->thread_ctx->state = TS_EXIT;
		BUG_ON(container_of(thread, struct object,
				    opaque)->refcount != 1);
		obj_put(thread);
		current_thread = NULL;
		BUG_ON(1);
	}
}

#define PFLAGS2VMRFLAGS(PF)                                                    \
	(((PF)&PF_X ? VMR_EXEC : 0) | ((PF)&PF_W ? VMR_WRITE : 0) |            \
	 ((PF)&PF_R ? VMR_READ : 0))

#define OFFSET_MASK (0xFFF)

/*
 * TODO: should we directly load the file to some pmo instead of load
 * content to some immediate buffer first
 */

/* load binary into some process (cap_group) */
static u64 load_binary(struct cap_group *cap_group,
		       struct vmspace *vmspace,
		       const char *bin,
		       struct process_metadata *metadata)
{
	struct elf_file *elf;
	vmr_prop_t flags;
	int i, r;
	size_t seg_sz, seg_map_sz;
	u64 p_vaddr;

	int *pmo_cap;
	struct pmobject *pmo;
	u64 ret;

	elf = elf_parse_file(bin);
	pmo_cap = kmalloc(elf->header.e_phnum * sizeof(*pmo_cap));
	if (!pmo_cap) {
		r = -ENOMEM;
		goto out_fail;
	}

	/* load each segment in the elf binary */
	for (i = 0; i < elf->header.e_phnum; ++i) {
		pmo_cap[i] = -1;
		if (elf->p_headers[i].p_type == PT_LOAD) {
			seg_sz = elf->p_headers[i].p_memsz;
			p_vaddr = elf->p_headers[i].p_vaddr;
			BUG_ON(elf->p_headers[i].p_filesz > seg_sz);
			seg_map_sz = ROUND_UP(seg_sz + p_vaddr, PAGE_SIZE)
			    - ROUND_DOWN(p_vaddr, PAGE_SIZE);

			pmo = obj_alloc(TYPE_PMO, sizeof(*pmo));
			if (!pmo) {
				r = -ENOMEM;
				goto out_free_cap;
			}
			pmo_init(pmo, PMO_DATA, seg_map_sz, 0);
			pmo_cap[i] = cap_alloc(cap_group, pmo, 0);
			if (pmo_cap[i] < 0) {
				r = pmo_cap[i];
				goto out_free_obj;
			}

			memset((void *)phys_to_virt(pmo->start), 0, pmo->size);
			memcpy((void *)phys_to_virt(pmo->start) +
			       (elf->p_headers[i].p_vaddr & OFFSET_MASK),
			       bin + elf->p_headers[i].p_offset,
			       elf->p_headers[i].p_filesz);

			flags = PFLAGS2VMRFLAGS(elf->p_headers[i].p_flags);

			ret = vmspace_map_range(vmspace,
						ROUND_DOWN(p_vaddr, PAGE_SIZE),
						seg_map_sz, flags, pmo);

			BUG_ON(ret != 0);
		}
	}

	/* return binary metadata */
	if (metadata != NULL) {
		metadata->phdr_addr = elf->p_headers[0].p_vaddr +
			            elf->header.e_phoff;
		metadata->phentsize = elf->header.e_phentsize;
		metadata->phnum     = elf->header.e_phnum;
		metadata->flags     = elf->header.e_flags;
		metadata->entry     = elf->header.e_entry;
	}

	kfree((void *)bin);

	/* PC: the entry point */
	return elf->header.e_entry;
out_free_obj:
	obj_free(pmo);
out_free_cap:
	for (--i; i >= 0; i--) {
		if (pmo_cap[i] != 0)
			cap_free(cap_group, pmo_cap[i]);
	}
out_fail:
	return r;
}

/* Defined in page_table.S (maybe required on aarch64) */
extern void flush_idcache(void);

/* Required by LibC */
extern void prepare_env(char *env, u64 top_vaddr,
			struct process_metadata *meta, char *name);
/*
 * This is for creating the first thread in the first (init) user process.
 * So, __create_root_thread needs to load the code/data as well.
 */
static int __create_root_thread(struct cap_group *cap_group, u64 stack_base,
			      u64 stack_size, u32 prio, u32 type, s32 aff,
			      const char *bin_start, char *bin_name)
{
	int ret, thread_cap, stack_pmo_cap;
	struct thread *thread;
	struct pmobject *stack_pmo;
	struct vmspace *init_vmspace;
	struct process_metadata meta;
	u64 stack;
	u64 pc;
	vaddr_t kva;

	init_vmspace = obj_get(cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
	obj_put(init_vmspace);

	/* Allocate and setup a user stack for the init thread */
	stack_pmo = obj_alloc(TYPE_PMO, sizeof(*stack_pmo));
	if (!stack_pmo) {
		ret = -ENOMEM;
		goto out_fail;
	}
	pmo_init(stack_pmo, PMO_ANONYM, stack_size, 0);
	stack_pmo_cap = cap_alloc(cap_group, stack_pmo, 0);
	if (stack_pmo_cap < 0) {
		ret = stack_pmo_cap;
		goto out_free_obj_pmo;
	}
	ret = vmspace_map_range(init_vmspace, stack_base, stack_size,
				VMR_READ | VMR_WRITE, stack_pmo);
	BUG_ON(ret != 0);

	/* Allocate the init thread */
	thread = obj_alloc(TYPE_THREAD, sizeof(*thread));
	if (!thread) {
		ret = -ENOMEM;
		goto out_free_cap_pmo;
	}

	/* Fill the parameter of the thread struct */
	pc = load_binary(cap_group, init_vmspace, bin_start, &meta);
	stack = stack_base + stack_size;

	/* Allocate a physical for the main stack for prepare_env */
	kva = (vaddr_t)get_pages(0);
	BUG_ON(kva == 0);
	commit_page_to_pmo(stack_pmo, stack_size / PAGE_SIZE - 1,
			   virt_to_phys((void *)kva));

	prepare_env((char *)kva, stack, &meta, bin_name);
	stack -= ENV_SIZE_ON_STACK;

	ret = thread_init(thread, cap_group, stack, pc, prio, type, aff);
	BUG_ON(ret != 0);

	/* Add the thread into the thread_list of the cap_group */
	lock(&cap_group->threads_lock);
	list_add(&thread->node, &cap_group->thread_list);
	cap_group->thread_cnt += 1;
	unlock(&cap_group->threads_lock);


	/* Allocate the cap for the init thread */
	thread_cap = cap_alloc(cap_group, thread, 0);
	if (thread_cap < 0) {
		ret = thread_cap;
		goto out_free_obj_thread;
	}

	/* L1 icache & dcache have no coherence on aarch64 */
	/* TODO (tmac): actually, I am not sure whether this is necessary here */
	flush_idcache();

	return thread_cap;
out_free_obj_thread:
	obj_free(thread);
out_free_cap_pmo:
	cap_free(cap_group, stack_pmo_cap);
	stack_pmo = NULL;
out_free_obj_pmo:
	obj_free(stack_pmo);
out_fail:
	return ret;
}

void *cpio_cb_file(const void *start, size_t size, void *data)
{
	char *buff = kmalloc(size);

	if (buff <= 0)
		return ERR_PTR(-ENOMEM);

	memcpy(buff, start, size);

	return buff;
}

/* load the binary of some file form tmpfs */
static int read_bin_from_fs(char *path, char **buf)
{
	BUG_ON(path == NULL);

	int ret = 0;

	*buf = cpio_extract_single(&binary_init_cpio_bin_start,
				   path,
				   cpio_cb_file,
				   NULL);

	if (ret == 0)
		return 0;
	else
		return -ENOSYS;
}

/*
 * exported functions
 */
void switch_thread_vmspace_to(struct thread *thread)
{
	switch_vmspace_to(thread->vmspace);
}

/* Arguments for the inital thread */
#define ROOT_THREAD_STACK_BASE		(0x500000000000UL)
#define ROOT_THREAD_STACK_SIZE		(0x800000UL)
#define ROOT_THREAD_PRIO		MAX_PRIO - 1

char ROOT_NAME[] = "/init.bin";

/*
 * The root_thread is actually a first user thread
 * which has no difference with other user threads
 */
void create_root_thread(void)
{
	struct cap_group *root_cap_group;
	int thread_cap;
	struct thread *root_thread;
	char *binary = NULL;
	int ret;

	/* Read from fs */
	ret = read_bin_from_fs(ROOT_NAME, &binary);
	BUG_ON(ret < 0);
	BUG_ON(binary == NULL);

	root_cap_group = create_root_cap_group(ROOT_NAME, strlen(ROOT_NAME));
	thread_cap =
	    __create_root_thread(root_cap_group, ROOT_THREAD_STACK_BASE,
			       ROOT_THREAD_STACK_SIZE, ROOT_THREAD_PRIO,
			       TYPE_USER, smp_get_cpu_id(), binary,
			       ROOT_NAME);

	root_thread = obj_get(root_cap_group, thread_cap, TYPE_THREAD);
	/* Enqueue: put init thread into the ready queue */
	BUG_ON(sched_enqueue(root_thread));
	obj_put(root_thread);
}

/*
 * create a thread in some process
 * return the thread_cap in the target cap_group
 * TODO: run/stop control; whether the cap in current cap_group is required
 */

static int create_thread(struct cap_group *cap_group,
		  u64 stack, u64 pc, u64 arg, u32 prio, u32 type, u64 tls)
{
	struct thread *thread;
	int cap, ret = 0;

	if (!cap_group) {
		ret = -ECAPBILITY;
		goto out_fail;
	}
	thread = obj_alloc(TYPE_THREAD, sizeof(*thread));
	if (!thread) {
		ret = -ENOMEM;
		goto out_obj_put;
	}
	ret = thread_init(thread, cap_group, stack, pc, prio, type, NO_AFF);
	if (ret != 0)
		goto out_free_obj;

	lock(&cap_group->threads_lock);
	list_add(&thread->node, &cap_group->thread_list);
	cap_group->thread_cnt += 1;
	unlock(&cap_group->threads_lock);

	arch_set_thread_arg(thread, arg);
	/* set thread tls */
	arch_set_thread_tls(thread, tls);
	/* cap is thread_cap in the target cap_group */
	cap = cap_alloc(cap_group, thread, 0);
	if (cap < 0) {
		ret = cap;
		goto out_free_obj;
	}
	/* ret is thread_cap in the current_cap_group */
	cap = cap_copy(cap_group, current_cap_group, cap, 0, 0);
	if (type == TYPE_USER) {
		thread->thread_ctx->state = TS_INTER;
		BUG_ON(sched_enqueue(thread));
	} else if (type == TYPE_SHADOW) {
		thread->thread_ctx->state = TS_WAITING;
	}
	return cap;

out_free_obj:
	obj_free(thread);
out_obj_put:
	obj_put(cap_group);
out_fail:
	return ret;
}

/**
 * FIXME(MK): This structure is duplicated in chcore musl headers.
 */
struct thread_args {
	u64 cap_group_cap;
	u64 stack;
	u64 pc;
	u64 arg;
	u32 prio;
	u64 tls;
	u32 is_shadow;
};

/*
 * Create a pthread in some process
 * return the thread_cap in the target cap_group
 * TODO: run/stop control; whether the cap in current cap_group is required
 */
int sys_create_thread(u64 thread_args_p)
{
	struct thread_args args = {0};
	struct cap_group *cap_group;
	int thread_cap;
	int r;
	u32 type;

	r = copy_from_user((char *)&args, (char *)thread_args_p, sizeof(args));
	BUG_ON(r);

	cap_group = obj_get(current_cap_group, args.cap_group_cap,
			    TYPE_CAP_GROUP);

	type = (args.is_shadow == 1) ? TYPE_SHADOW : TYPE_USER;
	thread_cap = create_thread(cap_group, args.stack, args.pc,
				   args.arg, args.prio, type,
				   args.tls);

	obj_put(cap_group);
	return thread_cap;
}

/* TODO: consider hide/remove/simplify the above interfaces later */

/* Exit the current running thread */
void sys_thread_exit(void)
{
	int cnt;

	/* Set thread state */
	current_thread->thread_ctx->state = TS_EXIT;

	/* As a normal application, the main thread will eventually invoke
	 * sys_exit_group or trigger unrecoverable fault (e.g., segfault).
	 *
	 * However a malicious application, all of its thread may invoke
	 * sys_thread_exit. So, we monitor the number of non-shadow threads
	 * in a cap_group (as a user process now).
	 */

	lock(&(current_cap_group->threads_lock));
	cnt = --current_cap_group->thread_cnt;
	unlock(&(current_cap_group->threads_lock));

	if (cnt == 0) {
		/*
		 * Current thread is the last non_shadow_thread in the process,
		 * so we invoke sys_exit_group.
		 */
		sys_exit_group(0);
		/* The control flow will not go through */
	}


	/* Set current running thread to NULL */
	current_thread = NULL;
	/* Reschedule */
	sched();
	eret_to_thread(switch_context());
}

int sys_set_affinity(u64 thread_cap, s32 aff)
{
	struct thread *thread = NULL;
	int ret = 0;

	/* XXX: currently, we use -1 to represent the current thread */
	if (thread_cap == -1) {
		thread = current_thread;
		BUG_ON(!thread);
	} else {
		thread = obj_get(current_cap_group, thread_cap, TYPE_THREAD);
	}

	if (thread == NULL) {
		ret = -ECAPBILITY;
		goto out;
	}

	/* Check aff */
	if (aff >= PLAT_CPU_NUM) {
		ret = -EINVAL;
		goto out_obj_put;
	}
	thread->thread_ctx->affinity = aff;
out_obj_put:
	if (thread_cap != -1)
		obj_put((void *)thread);
out:
	return ret;
}

s32 sys_get_affinity(u64 thread_cap)
{
	struct thread *thread = NULL;
	s32 aff = 0;

	/* XXX: currently, we use -1 to represent the current thread */
	if (thread_cap == -1) {
		thread = current_thread;
		BUG_ON(!thread);
	} else {
		thread = obj_get(current_cap_group, thread_cap, TYPE_THREAD);
	}

	if (thread == NULL)
		return -ECAPBILITY;

	aff = thread->thread_ctx->affinity;

	if (thread_cap != -1)
		obj_put((void *)thread);
	return aff;
}
