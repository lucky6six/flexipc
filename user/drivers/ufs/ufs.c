/**
 * UFS driver. Only one device is supported now.
 */

#include <chcore/type.h>
#include <chcore/defs.h>
#include <chcore/ipc.h>
#include <chcore/syscall.h>
#include <stdio.h>
#include <chcore/launcher.h>
//#include "linux-port/ufshci.h"
#include <stdlib.h>
#include <endian.h>
//#include "linux-port/ufshcd.h"
#include "ufs-bitfields.gen.h"
#include "ufshci.h"

#define PREFIX "[ufs]"

#define info(fmt, ...) printf(PREFIX " " fmt, ## __VA_ARGS__)
#define error(fmt, ...) printf(PREFIX " " fmt, ## __VA_ARGS__)

struct ufs_host host;

int ufs_init(void *unused, void *mmio_base, int irq)
{
	// struct usf_hba *hba = malloc(sizeof(*hba));
	// int ufs_version = *(uint32_t *)(mmio_base + REG_UFS_VERSION);

	// printf("ufs_version: %x\n", ufs_version);
	host.regmap = mmio_base;
	extern int ufs_host_controller_init(struct ufs_host *);

	ufs_host_controller_init(&host);
	// ufshcd_init(hba, mmio_base, irq);

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
	info("exit now. Bye!\n");
#endif

	usys_exit(0);
}

int main(int argc, char *argv[], char *envp[])
{
	// TODO
	// TODO: this file seems a duplication of user/drivers/ufs/ufs.c
	return 0;
}
