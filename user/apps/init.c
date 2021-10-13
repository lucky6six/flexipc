#include <chcore/defs.h>
#include <stdio.h>
#include <sys/mman.h>
#include <chcore/syscall.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <chcore/proc.h>
#include <chcore/ipc.h>
#include <chcore/fs_defs.h>
#include <chcore/elf.h>
#include <chcore/bug.h>
#include <chcore/launcher.h>
#include <chcore/launch_kern.h>
#include <chcore/cpio.h>
#include <chcore/procmgr_defs.h>

#define BUFLEN 4096
#define NR_ARGS_MAX 64

// TODO: remove all the tricky addresses
// TODO: keeping ls seems to exhaust memory

#ifndef FSM_SCAN_BUF_VADDR
#define FSM_SCAN_BUF_VADDR 0x30000000
#endif

static char buf[BUFLEN];

static int fsm_scan_pmo_cap;

/* fs_server_cap in current process; can be copied to others */
extern int fs_server_cap;
/* lwip_server_cap in current process; can be copied to others */
extern int lwip_server_cap;

extern ipc_struct_t *procmgr_ipc_struct;
extern int procmgr_server_cap;

int do_complement(char *buf, char *complement, int complement_time)
{
	int start = 0, ret = 0, len = 0, i = 0, j = 0;
	void *vp;
	struct dirent *p;
	char name[BUFLEN];
	char path[BUFLEN];
	ipc_msg_t *ipc_msg;
	struct fs_request fr;
	int r = -1;

	/* XXX: only support '/' here */
	strcpy(path, "/");

	/* IPC send cap */
	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 1);
	ipc_set_msg_cap(ipc_msg, 0, fsm_scan_pmo_cap);
	fr.req = FS_REQ_SCAN;
	fr.offset = start;
	fr.count = BUFLEN;
	strcpy((void *)fr.path, path);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	ret = ipc_call(fs_ipc_struct, ipc_msg);

	switch (ret) {
	case -ENOENT:
		break;
	case -ENOTDIR:
		break;
	default:
		while (ret > 0) {
			vp = (void *)FSM_SCAN_BUF_VADDR;
			start += ret;
			for (i = 0; i < ret; i++) {
				p = vp;
				len = p->d_reclen - sizeof(p->d_ino) -
				      sizeof(p->d_off) - sizeof(p->d_reclen) -
				      sizeof(p->d_type);
				memcpy(name, p->d_name, len);
				name[len - 1] = '\0';
				/* Compare the file name with the buf here */
				if (strstr(name, buf) != NULL) {
					/* multiple tab want to find different
					 * alternatives */
					if (j < complement_time) {
						j++;
					} else {
						strcpy(complement, name);
						r = 0;
						break;
					}
				}
				vp += p->d_reclen;
			}
			ipc_set_msg_cap(ipc_msg, 0, fsm_scan_pmo_cap);
			fr.offset = start;
			ipc_set_msg_data(ipc_msg, (char *)&fr, 0,
					 sizeof(struct fs_request));
			ret = ipc_call(fs_ipc_struct, ipc_msg);
		}
	}
	ipc_destroy_msg(ipc_msg);
	return r;
}

/* blocking getchar */
char getch()
{
	signed char c;
	do {
		c = getchar();
	} while (c == EOF);
	return c;
}

char *readline(const char *prompt)
{
	int i = 0, j = 0;
	signed char c = 0;
	int ret = 0;
	char complement[BUFLEN];
	int complement_time = 0;

	if (prompt != NULL) {
		printf("%s", prompt);
		fflush(stdout);
	}

	while (1) {
retry:
		c = getch();
	handle_new_char:
		if (c < 0) {
			/* Chinese input may lead to error */
			goto retry;
		} else if (c == '\b' || c == '\x7f') {
			if (i == 0)
				continue;
			usys_putc('\b');
			usys_putc(' ');
			usys_putc('\b');
			i--;
			buf[i] = 0;
		} else if (c >= ' ' && i < BUFLEN - 1) {
			usys_putc(c);
			buf[i++] = c;
		} else if (c == '\n' || c == '\r') {
			usys_putc('\r');
			usys_putc('\n');
			buf[i] = 0;
			break;
		} else if (c == 27) {
			getch();
			getch();
		} else if (c == '\t') { /* auto complement */
			/* Complement init */
			complement_time = 0;
			buf[i] = 0;
			/* In case not find anything */
			strcpy(complement, buf);
			do {
				/* Handle tab here */
				ret = do_complement(buf, complement,
						    complement_time);
				/* No new findings */
				if (ret == -1) {
					/* Not finding anything */
					if (complement_time == 0) {
						c = getch();
						goto handle_new_char;
					}
					complement_time = 0;
					continue;
				}
				/* Return to the head of the line */
				for (j = 0; j < i; j++)
					printf("\b \b");
				printf("%s", complement);
				/* flush to show */
				fflush(stdout);
				/* Update i */
				i = strlen(complement);
				/* Find a different next time */
				complement_time++;
				/* Get next char */
				c = getch();
			} while (c == '\t');
			if (c == '\n' || c == '\r') { /* End of input */
				strcpy(buf, complement);
				break;
			} else {
				strcpy(buf, complement);
				/* Get new char, has to handle */
				goto handle_new_char;
			}
		}
	}
	return buf;
}

int do_cd(char *cmdline)
{
	cmdline += 2;
	while (*cmdline == ' ')
		cmdline++;
	if (*cmdline == '\0')
		return 0;
	if (*cmdline != '/') {
	}
	printf("Build-in command cd %s: Not implemented!\n", cmdline);
	return 0;
}

int do_top()
{
	usys_top();
	return 0;
}

void fs_scan(char *path)
{
	int start = 0, ret = 0, len = 0, i = 0;
	void *vp;
	struct dirent *p;
	char name[256];
	ipc_msg_t *ipc_msg;
	struct fs_request fr;

	/* Path check */
	if (path[0] == '\0') {
		strcpy(path, "/");
	} else if (path[0] != '/') {
		strcpy(name, path);
		strcpy(path, "/");
		strcat(path, name);
	}
	printf("Path: %s\n", path);

	/* IPC send cap */
	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 1);
	ipc_set_msg_cap(ipc_msg, 0, fsm_scan_pmo_cap);
	fr.req = FS_REQ_SCAN;
	/* TODO(YJF): need malloc? */
	fr.offset = start;
	fr.count = BUFLEN;
	strcpy((void *)fr.path, path);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	ret = ipc_call(fs_ipc_struct, ipc_msg);

	switch (ret) {
	case -ENOENT:
		printf("%s :No such file or directory\n", path);
		break;
	case -ENOTDIR:
		printf("%s", path);
		break;
	default:
		while (ret > 0) {
			vp = (void *)FSM_SCAN_BUF_VADDR;
			start += ret;
			for (i = 0; i < ret; i++) {
				p = vp;
				len = p->d_reclen - sizeof(p->d_ino) -
				      sizeof(p->d_off) - sizeof(p->d_reclen) -
				      sizeof(p->d_type);
				memcpy(name, p->d_name, len);
				name[len - 1] = '\0';
				printf("%s\n", name);
				vp += p->d_reclen;
			}
			ipc_set_msg_cap(ipc_msg, 0, fsm_scan_pmo_cap);
			fr.offset = start;
			ipc_set_msg_data(ipc_msg, (char *)&fr, 0,
					 sizeof(struct fs_request));
			ret = ipc_call(fs_ipc_struct, ipc_msg);
		}
	}
	ipc_destroy_msg(ipc_msg);
}

int do_ls(char *cmdline)
{
	char pathbuf[BUFLEN];

	pathbuf[0] = '\0';
	cmdline += 2;
	while (*cmdline == ' ')
		cmdline++;
	strcat(pathbuf, cmdline);
	fs_scan(pathbuf);
	return 0;
}

void do_clear(void)
{
	usys_putc(12);
	usys_putc(27);
	usys_putc('[');
	usys_putc('2');
	usys_putc('J');
}

int builtin_cmd(char *cmdline)
{
	int ret, i;
	char cmd[BUFLEN];
	for (i = 0; cmdline[i] != ' ' && cmdline[i] != '\0'; i++)
		cmd[i] = cmdline[i];
	cmd[i] = '\0';
	if (!strcmp(cmd, "quit") || !strcmp(cmd, "exit"))
		usys_exit(0);
	if (!strcmp(cmd, "cd")) {
		ret = do_cd(cmdline);
		return !ret ? 1 : -1;
	}
	if (!strcmp(cmd, "ls")) {
		ret = do_ls(cmdline);
		return !ret ? 1 : -1;
	}
	if (!strcmp(cmd, "clear")) {
		do_clear();
		return 1;
	}
	if (!strcmp(cmd, "top")) {
		ret = do_top();
		return !ret ? 1 : -1;
	}
	return 0;
}
static inline bool is_whitespace(char ch)
{
	return ch == ' ' || ch == '\t';
}

static int parse_args(char *cmdline, char *pathbuf, char *argv[])
{
	int nr_args = 0;

	pathbuf[0] = '\0';
	while (*cmdline == ' ')
		cmdline++;

	if (*cmdline == '\0')
		return -ENOENT;

	if (*cmdline != '/')
		strcpy(pathbuf, "/");

	/* Copy to pathbuf. The length is a trick. */
	strncat(pathbuf, cmdline, BUFLEN - 1);
	pathbuf[BUFLEN] = '\0';

	while (*pathbuf) {

		/* Skip all spaces. */
		while (*pathbuf && is_whitespace(*pathbuf))
			pathbuf++;

		if (!*pathbuf)
			/* End of string. */
			break;

		argv[nr_args++] = pathbuf;

		while (*pathbuf && !is_whitespace(*pathbuf))
			pathbuf++;

		if (*pathbuf) {
			assert(is_whitespace(*pathbuf));
			*pathbuf = '\0';
			pathbuf++;
		}
	}

	return nr_args;
}

u64 get_badge_during_init(void)
{
	static int init_badge = FIXED_BADGE_DURING_INIT_BASE;
	if (init_badge >= FIXED_BADGE_DURING_INIT_END) {
		printf("[init] Badge exhausted during init!"
		       " Badge may overlap!\n");
	}
	return init_badge++;
}

int run_cmd(char *cmdline)
{
	char pathbuf[BUFLEN];
	char *argv[NR_ARGS_MAX];
	int argc;

	argc = parse_args(cmdline, pathbuf, argv);
	if (argc < 0) {
		printf("[Shell] Parsing arguments error\n");
		return argc;
	}

	return chcore_new_process(argc, argv, false);
}

void boot_fs(void)
{
	u64 size;

	int ret;
	int fsm_main_thread_cap;

	ret = 0;

	size = usys_fs_load_cpio(FSM_CPIO, QUERY_SIZE, 0);
	kernel_cpio_bin = mmap(0, ROUND_UP(size, PAGE_SIZE),
			       PROT_READ | PROT_WRITE,
			       MAP_PRIVATE | MAP_ANONYMOUS,
			       -1, 0);
	if (kernel_cpio_bin == (void*)-1) {
		printf("Failed: mmap for loading fsm_cpio from kernel.\n");
		usys_exit(-1);
	}
	usys_fs_load_cpio(FSM_CPIO, LOAD_CPIO, kernel_cpio_bin);

	/* create a new process */
	printf("Booting file system manager (fsm)...\n");
	ret = run_cmd_from_kernel_cpio("/fsm.srv", &fsm_main_thread_cap,
				       NULL, 0, get_badge_during_init());
	fail_cond(ret != 0, "create_process returns %d\n", ret);

	fs_server_cap = fsm_main_thread_cap;

	/* register IPC client */
	ipc_struct_t *tmp;
	tmp = ipc_register_client(fsm_main_thread_cap);
	fail_cond(!tmp, "ipc_register_client failed\n");
	memcpy(fs_ipc_struct, tmp, sizeof(*tmp) -
	       sizeof(enum system_server_identifier));

	fsm_scan_pmo_cap = usys_create_pmo(PAGE_SIZE, PMO_DATA);
	fail_cond(fsm_scan_pmo_cap < 0, "usys create_ret ret %d\n",
		  fsm_scan_pmo_cap);

	ret = usys_map_pmo(SELF_CAP, fsm_scan_pmo_cap, FSM_SCAN_BUF_VADDR,
			   VM_READ | VM_WRITE);
	fail_cond(ret < 0, "usys_map_pmo ret %d\n", ret);

	printf("[init] FS server is started.\n");
}

// TODO: remove arbitrary addressess
//#define DRVHUB_INFO_VADDR 0x88900000
//#define DTB_FILE_VADDR 0x88800000
void boot_driver_hub(void)
{
#if 0
	int err;
	int ret = 0;
	struct pmo_map_request pmo_map_reqs[2];
	ipc_msg_t *ipc_msg;
	struct fs_request fr;
	size_t file_size;
	size_t pmo_size;
	int dtb_pmo_cap;
	int info_pmo_cap;
	struct info_page *info_page;
	int caps[1];

	// Get file size
	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 1);
	fr.req = FS_REQ_GET_SIZE;
	strncpy(fr.path, "/kirin970-hikey970.dtb", FS_REQ_PATH_LEN);
	fr.path[FS_REQ_PATH_LEN] = '\0';
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	file_size = ipc_call(fs_ipc_struct, ipc_msg);
	printf("dtb file size: %d\n", file_size);

	// Get file data
	pmo_size = ROUND_UP(file_size, PAGE_SIZE);
	dtb_pmo_cap = usys_create_pmo(pmo_size, PMO_DATA);
	fail_cond(dtb_pmo_cap < 0, "usys_create_pmo failed %d\n", dtb_pmo_cap);

	ret = usys_map_pmo(SELF_CAP, dtb_pmo_cap, DTB_FILE_VADDR,
			   VM_READ | VM_WRITE);

	fail_cond(ret < 0, "usys_map_pmo ret %d\n", ret);

	fr.req = FS_REQ_READ;
	fr.offset = 0;
	fr.count = file_size;
	ipc_set_msg_cap(ipc_msg, 0, dtb_pmo_cap);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	ret = ipc_call(fs_ipc_struct, ipc_msg);

	printf("dtb file data loaded\n");

	ipc_destroy_msg(ipc_msg);

	// Start driver hub
	/* Now the dtb file stores in the dtb_pmo */
	/* Prepare info page */
	info_pmo_cap = usys_create_pmo(PAGE_SIZE, PMO_DATA);
	fail_cond(info_pmo_cap < 0, "usys_create_ret ret %d\n", info_pmo_cap);

	ret = usys_map_pmo(SELF_CAP, info_pmo_cap, DRVHUB_INFO_VADDR,
			   VM_READ | VM_WRITE);
	fail_cond(ret < 0, "usys_map_pmo ret %d\n", ret);

	info_page = (void *)DRVHUB_INFO_VADDR;
	info_page->ready_flag = false;
	info_page->nr_args = 1;
	info_page->args[0] = DTB_FILE_VADDR;

	pmo_map_reqs[0].pmo_cap = info_pmo_cap;
	pmo_map_reqs[0].addr = DRVHUB_INFO_VADDR;
	pmo_map_reqs[0].perm = VM_READ | VM_WRITE;
	pmo_map_reqs[1].pmo_cap = dtb_pmo_cap;
	pmo_map_reqs[1].addr = DTB_FILE_VADDR;
	pmo_map_reqs[1].perm = VM_READ | VM_WRITE;
	caps[0] = fs_server_cap;
	err = launch_process_path("/driver_hub.srv", NULL, pmo_map_reqs, 2,
				  caps, 1, get_badge_during_init());
	fail_cond(err != 0, "create_process returns %d\n", err);

	while (!info_page->ready_flag)
		usys_yield();

	/* TODO(MK): How to reclaim the pmos? */
#endif
}

static void boot_kservice(const char *srv_name, const char *srv_path,
			  int *srv_cap_p, int nr_caps, int caps[])
{
	struct user_elf user_elf;
	int ret;
	int main_thread_cap;
	char *argv[1];

	struct launch_process_args lp_args;

	const char *filename = srv_path;

	/* create a new process */
	printf("Booting %s...\n", srv_name);

	ret = readelf_from_fs(filename, &user_elf);
	if (ret < 0) {
		printf("[Shell] No such binary in FS.\n");
		usys_exit(ret);
		return;
	}

	/*
	 * Check the string length.
	 * Do not forget the one more byte for the ending '\0'.
	 */
	if (strlen(filename) + strlen(KERNEL_SERVER) >= ELF_PATH_LEN) {
		printf("%s failed due to the long filename (support at most %ld bytes).\n",
		       __func__, ELF_PATH_LEN - strlen(KERNEL_SERVER) - 1);
		usys_exit(-1);
		return;
	}

	/* Launching a system server: add a prefix (as token) to the pathname */
	snprintf(user_elf.path, ELF_PATH_LEN, "%s/%s", KERNEL_SERVER, filename);
	argv[0] = user_elf.path;

	lp_args.user_elf = &user_elf;
	lp_args.child_process_cap = NULL;
	lp_args.child_main_thread_cap = &main_thread_cap;
	lp_args.pmo_map_reqs = NULL;
	lp_args.nr_pmo_map_reqs = 0;
	lp_args.caps = caps;
	lp_args.nr_caps = nr_caps;
	lp_args.cpuid = 0;
	lp_args.argc = 1;
	lp_args.argv = argv;
	lp_args.badge = get_badge_during_init();
	/* Note: kservice has no PID */
	lp_args.pid = 0;

	ret = launch_process_with_pmos_caps(&lp_args);

	fail_cond(ret != 0, "launch %s server returns %d\n", srv_name, ret);

	if (srv_cap_p)
		*srv_cap_p = main_thread_cap;

	printf("[INIT] %s is started.\n", srv_name);
}

/* Init is the first user process and there is no system services now.
 * So, override the default libc_connect_services.
 */
void libc_connect_services(char *envp[])
{
	/* Do nothing. */
	return;
}

/**
 * Process Manager needs to be initialized via IPC for two reasons:
 * 1. A process created by procmgr should be able to create new proceses (via
 *    procmgr) by default. To enable this, the new process should know the
 *    capability of procmgr so that it could send IPC to procmgr. However,
 *    procmgr does not know its own capability (the capability used to connect
 *    to procmgr), thus it cannot copy the procmgr capability to new processes.
 *    Here we solve this using a hack where the init app, who created the
 *    procmgr, sends the procmgr capability to procmgr explicitly, so that
 *    procmgr could send the procmgr capability to new processes.
 * 2. The init app is created before procmgr. The first IPC connection to the
 *    procmgr notifies procmgr who is the init app.
 */
int init_procmgr(void)
{
	ipc_msg_t *ipc_msg;
	int ret;
	struct proc_request pr;

	/* register IPC client */
	procmgr_ipc_struct = ipc_register_client(procmgr_server_cap);
	fail_cond(!procmgr_ipc_struct, "ipc_register_client failed\n");

	ipc_msg = ipc_create_msg(procmgr_ipc_struct,
				 sizeof(struct proc_request), 1);
	pr.req = PROC_REQ_INIT;

    /**
     * Copy the procmgr cap to procgmr, so that procmgr could copy the cap to
     * new processes when creating new proceses.
     */
	ipc_set_msg_cap(ipc_msg, 0, procmgr_server_cap);
	ipc_set_msg_data(ipc_msg, &pr, 0, sizeof(pr));
	ret = ipc_call(procmgr_ipc_struct, ipc_msg);

	ipc_destroy_msg(ipc_msg);
	return ret;
}

int main(int argc, char *argv[])
{
	char *buf;
	int ret = 0;
	int caps[2];
	int nr_caps = 0;

	/* Do not modify the order of creating system servers */

	/*
	 * This is the first user process launched by the kernel.
	 * Boot system servers of ChCore first.
	 */

	printf("User Init: booting fs server (FSMGR and real FS) \n");
	/* FSM gets badge 2 and tmpfs uses the fixed badge (10) for it */
	boot_fs();
	caps[nr_caps++] = fs_server_cap;

	// boot_driver_hub();

	printf("User Init: booting network server\n");
	/* Pass the FS cap to NET since it may need to read some config files */
	/* Net gets badge 3 */
	boot_kservice("network", "/lwip.srv", &lwip_server_cap, nr_caps, caps);
	caps[nr_caps++] = lwip_server_cap;


	printf("User Init: booting procmgr server\n");
	/*
	 * procmgr requires the FS cap and NET cap because it needs to
	 * pass these caps to applications.
	 */
	/* Proc gets badge 4 */
	boot_kservice("procmgr", "/procmgr.srv", &procmgr_server_cap, nr_caps, caps);
	init_procmgr();


	/* After booting system servers, run the ChCore shell. */
	printf("\nWelcome to ChCore shell!");
	while (1) {
		printf("\n");
		buf = readline("$ ");
		if (buf == NULL)
			usys_exit(0);
		if (buf[0] == 0)
			continue;
		if (builtin_cmd(buf))
			continue;
		if ((ret = run_cmd(buf)) < 0) {
			printf("Cannot run %s, ERROR %d\n", buf, ret);
		}
	}

	return 0;
}
