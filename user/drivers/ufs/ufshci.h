#pragma once

#include "type.h"

/* UTP Command Descriptor w.r.t. JESD223C $6.1.2. */
struct utp_cmddes {
	char command_upiu[64];
	char response_upiu[64];
	/* TODO: Physical Region Description Table (optional) */
} __attribute__((aligned(PAGE_SIZE)));

struct ufs_host {

	volatile void *regmap;

	/* We allocate a single page for tskmgmt and txfr request descriptors.
	   The following two reqdes pointers just point to this page. */
	int utp_page_pmo_cap;
	void *utp_page;
	paddr_t utp_page_paddr;

#define MAX_NR_UTP_TSKMGMT_REQDES 8
#define MAX_NR_UTP_TXFR_REQDES 32
	int nr_utp_txfr_reqdes;
	int nr_utp_tskmgmt_reqdes;
	struct utp_txfr_reqdes *utp_txfr; /* UTP transfer request list */
	struct utp_tskmgmt_reqdes *utp_tskmgmt; /* UTP task management request list */

	struct utp_cmddes *utp_cmds;

};
int ufs_read(struct ufs_host *ufshost, void *buf4k, int block_nr);
int ufs_write(struct ufs_host *ufshost, void *buf4k, int block_nr);
