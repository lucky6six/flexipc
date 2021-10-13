#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chcore/bug.h>
#include <chcore/defs.h>
#include <chcore/elf.h>
#include <chcore/fs_defs.h>
#include <chcore/procmgr_defs.h>
#include <chcore/proc.h>
#include <chcore/ipc.h>
#include <chcore/syscall.h>
#include <chcore/launcher.h>

static int fs_read(const char *path, char **binary)
{
	int fd, ret, byte_read;
	struct stat statbuf;

	if ((fd = openat(AT_FDCWD, path, O_RDONLY)) < 0) {
		return -1;
	}

	if ((ret = fstat(fd, &statbuf)) < 0) {
		// TODO: close(fd);
		return -1;
	}

	if ((*binary = malloc(statbuf.st_size)) == NULL) {
		// TODO: close(fd);
		return -1;
	}

	byte_read = 0;
	while (byte_read != statbuf.st_size) {
		if ((ret = read(fd, *binary + byte_read,
				statbuf.st_size - byte_read)) < 0) {
			// TODO: close(fd);
			return -1;
		}

		byte_read += ret;
	}

	// TODO: close(fd);

	return 0;
}
#define VMR_READ (1 << 0)
#define VMR_WRITE (1 << 1)
#define VMR_EXEC (1 << 2)

#define PFLAGS2VMRFLAGS(PF)                                                    \
	(((PF)&PF_X ? VMR_EXEC : 0) | ((PF)&PF_W ? VMR_WRITE : 0) |            \
	 ((PF)&PF_R ? VMR_READ : 0))

#define OFFSET_MASK 0xfff

int parse_elf_from_binary(const char *binary, struct user_elf *user_elf)
{
	int ret;
	struct elf_file *elf;
	size_t seg_sz, seg_map_sz;
	u64 p_vaddr;
	int i;
	int j;
	// TODO: fuck
	u64 tmp_vaddr = 0xc00000;

	elf = elf_parse_file(binary);

	if ((elf->header.e_type != ET_EXEC) && (elf->header.e_type != ET_DYN)) {
		printf("%s is non-runnable.\n");
		return -1;
	}

	if ((elf->header.e_type == ET_DYN) &&
	    strncmp(user_elf->path, CHCORE_LOADER, strlen(CHCORE_LOADER))) {
		return ET_DYN;
	}

	/* init pmo, -1 indicates that this pmo is not used
	 *
	 * TODO(YJF): an elf file can have 2 PT_LOAD segment at most? */
	for (i = 0; i < 2; ++i)
		user_elf->user_elf_seg[i].elf_pmo = -1;

	for (i = 0, j = 0; i < elf->header.e_phnum; ++i) {
		if (elf->p_headers[i].p_type != PT_LOAD)
			continue;

		seg_sz = elf->p_headers[i].p_memsz;
		p_vaddr = elf->p_headers[i].p_vaddr;
		BUG_ON(elf->p_headers[i].p_filesz > seg_sz);
		seg_map_sz = ROUND_UP(seg_sz + p_vaddr, PAGE_SIZE) -
			     ROUND_DOWN(p_vaddr, PAGE_SIZE);

		user_elf->user_elf_seg[j].elf_pmo =
		    usys_create_pmo(seg_map_sz, PMO_DATA);
		BUG_ON(user_elf->user_elf_seg[j].elf_pmo < 0);

		ret = usys_map_pmo(SELF_CAP, user_elf->user_elf_seg[j].elf_pmo,
				   tmp_vaddr, VM_READ | VM_WRITE);
		BUG_ON(ret < 0);

		memset((void *)tmp_vaddr, 0, seg_map_sz);
		/*
		 * OFFSET_MASK is for calculating the final offset for loading
		 * different segments from ELF.
		 * ELF segment can specify not aligned address.
		 *
		 */
		memcpy((void *)(tmp_vaddr +
			   (elf->p_headers[i].p_vaddr & OFFSET_MASK)),
		       (void *)(binary + elf->p_headers[i].p_offset),
		       elf->p_headers[i].p_filesz);

		user_elf->user_elf_seg[j].seg_sz = seg_sz;
		user_elf->user_elf_seg[j].p_vaddr = p_vaddr;
		user_elf->user_elf_seg[j].flags =
		    PFLAGS2VMRFLAGS(elf->p_headers[i].p_flags);
		usys_unmap_pmo(SELF_CAP, user_elf->user_elf_seg[j].elf_pmo,
			       tmp_vaddr);

		j++;
	}

	user_elf->elf_meta.phdr_addr =
	    elf->p_headers[0].p_vaddr + elf->header.e_phoff;
	user_elf->elf_meta.phentsize = elf->header.e_phentsize;
	user_elf->elf_meta.phnum = elf->header.e_phnum;
	user_elf->elf_meta.flags = elf->header.e_flags;
	user_elf->elf_meta.entry = elf->header.e_entry;
	user_elf->elf_meta.type = elf->header.e_type;

	return elf->header.e_type;
}

int readelf_from_fs(const char *pathbuf, struct user_elf *user_elf)
{
	int ret;
	char *binary;

	if ((ret = fs_read(pathbuf, &binary)) < 0) {
		return ret;
	}

	strcpy(user_elf->path, pathbuf);
	ret = parse_elf_from_binary(binary, user_elf);

	if ((ret == ET_DYN) &&
	    (strncmp(pathbuf, CHCORE_LOADER, strlen(CHCORE_LOADER)))) {

		/* Execute libc.so and pass @pathbuf as an argument later */
		free(binary);

		memset((void *)user_elf, 0, sizeof(*user_elf));

		if ((ret = fs_read(CHCORE_LOADER, &binary)) < 0) {
			printf("func (%s) failed in loading CHCORE_LOADER (%s)\n",
			       __func__, CHCORE_LOADER);
			return ret;
		}
		strcpy(user_elf->path, CHCORE_LOADER);
		ret = parse_elf_from_binary(binary, user_elf);
		return ret;
	}

	return ret;
}

extern ipc_struct_t *procmgr_ipc_struct;

pid_t chcore_new_process(int argc, char *__argv[], int is_bbapplet)
{
	struct user_elf user_elf;
	int ret;
	int caps[3];
	const int MAX_ARGC = 128;
	char *argv[MAX_ARGC];
	int i;
	int argv_start;
	struct proc_request *pr;
	ipc_msg_t *ipc_msg;
	int text_i;

	assert(argc + 1 < PROC_REQ_ARGC_MAX);
	/*
	 * Reserve argv[0] for busybox applets.
	 * Dynamic loaders are handled by procmgr.
	 */
	argv_start = 1;
	for (i = 0; i < argc; ++i)
		argv[i + argv_start] = __argv[i];

	if (is_bbapplet) {
		/* This is a busybox applet. Invoke busybox. */
		argv[0] = "/busybox";
		argv_start = 0;
		argc += 1;
	}

	ipc_msg = ipc_create_msg(procmgr_ipc_struct,
				 sizeof(struct proc_request), 0);
	pr = (struct proc_request *)ipc_get_msg_data(ipc_msg);

	pr->req = PROC_REQ_NEWPROC;
	pr->argc = argc;

	/* Dump argv[] to the text area of the pr. */
	for (i = 0, text_i = 0; i < argc; ++i) {
		/* Plus 1 for the trailing \0. */
		int len = strlen(argv[argv_start + i]) + 1;
		assert(text_i + len <= PROC_REQ_TEXT_SIZE);

		memcpy(&pr->text[text_i], argv[argv_start + i], len);

		pr->argv[i] = text_i;
		text_i += len;
	}

	ret = ipc_call(procmgr_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int get_new_process_mt_cap(int pid)
{
	ipc_msg_t *ipc_msg;
	struct proc_request *pr;
	int mt_cap;

	ipc_msg = ipc_create_msg(procmgr_ipc_struct,
				 sizeof(struct proc_request), 1);

	pr = (struct proc_request *)ipc_get_msg_data(ipc_msg);
	pr->req = PROC_REQ_GET_MT_CAP;
	pr->pid = pid;

	ipc_call(procmgr_ipc_struct, ipc_msg);
	mt_cap = ipc_get_msg_cap(ipc_msg, 0);
	ipc_destroy_msg(ipc_msg);

	return mt_cap;
}

int create_process(int argc, char *__argv[], struct new_process_caps *caps)
{
	pid_t pid;
	pid = chcore_new_process(argc, __argv, 0);
	if (caps != NULL)
		caps->mt_cap = get_new_process_mt_cap(pid);
	return pid;
}
