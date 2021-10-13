/**
 * UFS driver. Only one device is supported now.
 */

#include <type.h>
#include <chcore/defs.h>
#include <chcore/ipc.h>
#include <syscall.h>
#include <stdio.h>
#include <chcore/launcher.h>
#include <stdlib.h>
#include <endian.h>

#define PREFIX "[ufs]"

#define info(fmt, ...) printf(PREFIX " " fmt, ## __VA_ARGS__)
#define error(fmt, ...) printf(PREFIX " " fmt, ## __VA_ARGS__)

#include "ufshcd.h"
struct ufs_hba hba;
char host_page[4096];

int ufs_init(void *unused, void *mmio_base, int irq)
{
	// struct usf_hba *hba = malloc(sizeof(*hba));
	// int ufs_version = *(uint32_t *)(mmio_base + REG_UFS_VERSION);

	// printf("ufs_version: %x\n", ufs_version);
	extern int ufshcd_init(struct ufs_hba *hba, void *mmio_base,
			       unsigned int irq);

	hba.host = (void *)host_page;
	ufshcd_init(&hba, mmio_base, 0x116+32);

	return 0;
}

void ufs_dispatch(ipc_msg_t *ipc_msg)
{
	int ret = 0;

	if (ipc_msg->data_len >= 4) {
	}
	else {
		printf("TMPFS: no operation num\n");
		usys_exit(-1);
	}

	ipc_return(ipc_msg, ret);
}

void ufs_serve(void *mmio)
{
	ufs_init(NULL, mmio, 0);
#if 0
	info("register server value = %u\n",
	     ipc_register_server(ufs_dispatch));
#endif
	usys_exit(0);
}

int main(int argc, char *argv[], char *envp[])
{
	// TODO
	return 0;
}
