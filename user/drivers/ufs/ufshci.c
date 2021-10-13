#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <syscall.h>
#include <string.h>
#include <chcore/ipc.h>
#include <chcore/bug.h>
#include <chcore/proc.h>
#include <chcore/defs.h>
#include <bits/errno.h>
#include "ufsutp.h"

#include "ufs-bitfields.gen.h"
#include "ufshci.h"

#define WAIT_UNTIL(cond) do { } while (!(cond))

#define hcimsg(fmt, ...) printf("[UFS HCI]: "fmt, ##__VA_ARGS__)

int lowest_free_bit(u32 bitset)
{
	int i;

	for (i = 0; i < 32; ++i)
		if (!(bitset & (1 << i)))
			return i;

	return -1;
}

int lowest_used_bit(u32 bitset)
{
	int i;

	for (i = 0; i < 32; ++i)
		if (bitset & (1 << i))
			return i;

	return -1;
}

/* WIP */
int ufs_write(struct ufs_host *host, void *buf4k, int block_nr)
{
	void *ucd;
	// 1. Find the free slot
	u32 doorbell = read_regmap_UTRLDBR(host->regmap);
	int slot = lowest_free_bit(doorbell);

	if (slot == -1)
		return -EBUSY;

	// 2. Build UTRD
	/* The UFS SCSI command set */
	write_utp_txfr_reqdes_CT(&host->utp_txfr[slot], 1);
	/* Write to device */
	write_utp_txfr_reqdes_DD(&host->utp_txfr[slot],
				 TXFR_DD_SYSMEM_TO_DEV);
	/* Do not use interrupt */
	write_utp_txfr_reqdes_I(&host->utp_txfr[slot], 0);
	/* Clear OCS. FIXME(MK): Do not use hardcoded... */
	write_utp_txfr_reqdes_OCS(&host->utp_txfr[slot], 0xF);
	// 2.e. Allocate and initilize a UCD
	ucd = malloc(PAGE_SIZE);
	fail_cond(ucd == NULL, "allocation failed");

	return 0;
}

/* WIP */

static int ufs_send_read_request(struct ufs_host *host, int block_nr)
{
	// According to doc JESD223C $7.2.1
	struct utp_txfr_reqdes *txfr;
	void *ucd;
	int ret;
	// 1. Find the free slot
	u32 doorbell = read_regmap_UTRLDBR(host->regmap);
	int slot = lowest_free_bit(doorbell);

	printf("slot: %d\n", slot);
	if (slot == -1)
		return -EBUSY;

	txfr = &host->utp_txfr[slot];
	// 2. Build UTRD
	/* The UFS SCSI command set */
	write_utp_txfr_reqdes_CT(txfr, 1);
	/* Write to device */
	write_utp_txfr_reqdes_DD(txfr, TXFR_DD_DEV_TO_SYSMEM);
	/* Do not use interrupt */
	write_utp_txfr_reqdes_I(txfr, 0);
	/* Clear OCS. FIXME(MK): Do not use hardcoded... */
	write_utp_txfr_reqdes_OCS(txfr, 0xF);
	// 2.e. Allocate and initilize a UCD

	ucd = &host->utp_cmds[slot];
	memset(ucd, 0, PAGE_SIZE);
	utp_build_command_upiu_read(ucd, block_nr);

	// 3. Set address of UCD
	paddr_t paddr;
	ret = usys_get_phys_addr(ucd, &paddr);
	fail_cond(ret < 0, "usys_get_phys_addr ret %d\n", ret);
	write_utp_txfr_reqdes_UCDBA(txfr, ADDR_LOWER32(paddr));
	write_utp_txfr_reqdes_UCDBAU(txfr, ADDR_UPPER32(paddr));
	// 4-5. Setup offset/length of response
	write_utp_txfr_reqdes_RUO(txfr, 64);
	write_utp_txfr_reqdes_RUL(txfr, 64);
	// 6-7. Setup offset/length of physical des table
	write_utp_txfr_reqdes_PRDTO(txfr, 128);
	write_utp_txfr_reqdes_PRDTL(txfr, 32);
	// No 8 in the doc
	// 9. check
	WAIT_UNTIL(read_regmap_UTRLRSR(host->regmap) == 1);
	// 10-13. Enable aggragated interrupt?
	/* NOTE(MK): The document might be wrong. This bit only controls whether
	 * the interrupts are aggragated (counted and timed), and it has nothing
	 * to do with enability of interrupts.
	 */
	write_regmap_UTRIACR_IAEN(host->regmap, 1);
	write_regmap_UTRIACR_CTR(host->regmap, 1);
	write_regmap_UTRIACR_IACTH(host->regmap, 1);
	write_regmap_UTRIACR_IATOVAL(host->regmap, 0xf);

	// 14. Ring the doorbell
	write_regmap_UTRLDBR(host->regmap, 1UL << slot);

	return 0;
}

int ufs_irq_cap;

int ufs_read(struct ufs_host *host, void *buf4k, int block_nr)
{
	volatile void *regmap = host->regmap;
	ufs_send_read_request(host, block_nr);
	while (1) {
		printf("Wait irq\n");
		usys_irq_wait(ufs_irq_cap, true);

		printf("UTRLDBR: 0x%llx\n",
		       read_regmap_UTRLDBR(regmap));
		printf("UTRLRSR: 0x%llx\n",
		       read_regmap_UTRLRSR(regmap));
		printf("UTRLCNR: 0x%llx\n",
		       read_regmap_UTRLCNR(regmap));
		u32 completion = read_regmap_UTRLCNR(regmap);
		int slot = lowest_used_bit(completion);
		printf("slot: %d\n", slot);
		struct utp_txfr_reqdes *txfr;
		struct utp_cmddes *ucd;

		txfr = &host->utp_txfr[slot];
		ucd = &host->utp_cmds[slot];
		printf("ocs: %d\n", read_utp_txfr_reqdes_OCS(txfr));

		printf("type: 0x%lx\n",
		       read_upiu_header_TxnType((void *)ucd + 64));

		/* Clear notification bit */
		write_regmap_UTRLCNR(regmap, 1UL << slot);
		/* Continue to next packet */
		WAIT_UNTIL(read_regmap_HCS_UTRLRDY(regmap) == 1);
		// write_regmap_UTRLCNR(regmap, 1UL << slot);

		printf("Waken up. Acking irq\n");
		usys_irq_ack(ufs_irq_cap);
		break;
	}
	return 0;
}


static inline void dump_dw(void *mem, int line)
{
	int i, j;
	u32 *dwp = mem;
	printf("      ");
	for (j = 31; j >= 0; --j) {
		printf(" %d", j%10);
	}
	printf("\n");
	for (i = 0; i < line; ++i, ++dwp) {
		u32 dw = *dwp;
		printf("dw%02d: ", i);
		for (j = 31; j >= 0; --j) {
			printf(" %d", (dw >> j) & 1);
		}
		printf("\n");
	}
}

/*
 * Test UFS setup by sending NOP OUT and receiving NOP IN.
 */
int ufs_test(struct ufs_host *host)
{
	// According to doc JESD223C $7.2.1
	struct utp_txfr_reqdes *txfr;
	volatile void *regmap = host->regmap;
	void *ucd;
	int ret;
	// 1. Find the free slot
	u32 doorbell = read_regmap_UTRLDBR(host->regmap);
	int slot = lowest_free_bit(doorbell);

	printf("slot: %d\n", slot);
	if (slot == -1)
		return -EBUSY;

	txfr = &host->utp_txfr[slot];
	memset(txfr, 0, PAGE_SIZE);
	// 2. Build UTRD
	/* The UFS SCSI command set */
	write_utp_txfr_reqdes_CT(txfr, 1);
	/* Write to device */
	write_utp_txfr_reqdes_DD(txfr, TXFR_DD_NONDATA);
	/* Do not use interrupt */
	write_utp_txfr_reqdes_I(txfr, 1);
	/* Clear OCS. FIXME(MK): Do not use hardcoded... */
	write_utp_txfr_reqdes_OCS(txfr, 0xF);
	// 2.e. Allocate and initilize a UCD

	ucd = &host->utp_cmds[slot];
	memset(ucd, 0, PAGE_SIZE); /* NOP OUT: all zero */
	// 3. Set address of UCD
	paddr_t paddr;
	ret = usys_get_phys_addr(ucd, &paddr);
	fail_cond(ret < 0, "usys_get_phys_addr ret %d\n", ret);
	write_utp_txfr_reqdes_UCDBA(txfr, ADDR_LOWER32(paddr));
	write_utp_txfr_reqdes_UCDBAU(txfr, ADDR_UPPER32(paddr));
	// 4-5. Setup offset/length of response
	write_utp_txfr_reqdes_RUO(txfr, 64);
	write_utp_txfr_reqdes_RUL(txfr, 64);
	// 6-7. Setup offset/length of physical des table
	write_utp_txfr_reqdes_PRDTO(txfr, 128);
	write_utp_txfr_reqdes_PRDTL(txfr, 0);
	// No 8 in the doc
	// 9. check
	WAIT_UNTIL(read_regmap_UTRLRSR(host->regmap) == 1);
	// 10-13. Enable aggragated interrupt?
	/* NOTE(MK): The document might be wrong. This bit only controls whether
	 * the interrupts are aggragated (counted and timed), and it has nothing
	 * to do with enability of interrupts.
	 */
	write_regmap_UTRIACR_IAEN(host->regmap, 1);
	write_regmap_UTRIACR_CTR(host->regmap, 1);
	write_regmap_UTRIACR_IACTH(host->regmap, 1);
	write_regmap_UTRIACR_IATOVAL(host->regmap, 0xf);


	printf("pre door bell\n");

		printf("UTRLDBR: 0x%llx\n",
		       read_regmap_UTRLDBR(regmap));
		printf("UTRLRSR: 0x%llx\n",
		       read_regmap_UTRLRSR(regmap));
		printf("UTRLCNR: 0x%llx\n",
		       read_regmap_UTRLCNR(regmap));

	dump_dw(txfr, 16);
	printf("ucd paddr: 0x%lx\n", paddr);
	dump_dw(ucd, 16);

	// 14. Ring the doorbell
	write_regmap_UTRLDBR(host->regmap, 1UL << slot);



	int r = 2;
	while (r--) {
		printf("Wait irq\n");
		usys_irq_wait(ufs_irq_cap, true);

		printf("UTRLDBR: 0x%llx\n",
		       read_regmap_UTRLDBR(regmap));
		printf("UTRLRSR: 0x%llx\n",
		       read_regmap_UTRLRSR(regmap));
		printf("UTRLCNR: 0x%llx\n",
		       read_regmap_UTRLCNR(regmap));
		u32 completion = read_regmap_UTRLCNR(regmap);
		int slot = lowest_used_bit(completion);
		printf("slot: %d\n", slot);
		struct utp_txfr_reqdes *txfr;
		struct utp_cmddes *ucd;

		txfr = &host->utp_txfr[slot];
		ucd = &host->utp_cmds[slot];
		printf("ocs: %d\n", read_utp_txfr_reqdes_OCS(txfr));

		printf("type: 0x%lx\n",
		       read_upiu_header_TxnType((void *)ucd + 64));

		/* Clear notification bit */
		write_regmap_UTRLCNR(regmap, 1UL << slot);
		/* Continue to next packet */
		WAIT_UNTIL(read_regmap_HCS_UTRLRDY(regmap) == 1);
		// write_regmap_UTRLCNR(regmap, 1UL << slot);

		printf("Waken up. Acking irq\n");
		usys_irq_ack(ufs_irq_cap);
	}

	return 0;
}

static inline void delay(void)
{
	// Sleep for a while
	int i;
	volatile int a = 1;
	for (i = 0; i < 9000000; ++i) {
		a = a / 1.222 * 12.12;
	}
	asm volatile("dmb ish":::"memory");
}



int ufs_host_controller_init(struct ufs_host *ufshost)
{
	int ret;
	int i;
	paddr_t paddr;
	volatile void *regmap = ufshost->regmap;

	hcimsg("ufs_version: %x\n", read_regmap_VER(regmap));
	/* The following write should not take effect on device memory. */
	write_regmap_VER(regmap, 0x10);
	hcimsg("ufs_version: %x\n", read_regmap_VER(regmap));

	/* Clear and disable interrupts. */
	write_regmap_IS(regmap, read_regmap_IS(regmap));
	write_regmap_IE(regmap, 0);

	asm volatile("dsb ish":::"memory");

	// 2. Basic init
	hcimsg("ok 2\n");
	hcimsg("hce: %d\n", read_regmap_HCE(regmap));
	/* NOTE(MK): The HCE can be 1 after initialization! We need to zero it! */
	write_regmap_HCE(regmap, 0);
	delay();
	hcimsg("hce: %d\n", read_regmap_HCE(regmap));
	write_regmap_HCE(regmap, 1);
	// 3. Wait basic init finish
	delay();
	WAIT_UNTIL(read_regmap_HCE(regmap) == 1);
	hcimsg("ok 3\n");

	// 4. Additional commands
	// 5. Optionally enable IS.UCCS interrupt
	/* NOTE(MK): We use polling here, and enable UCCE afterwards. */
	// write_regmap_IE_UCCE(regmap, 1);
retry6:
	hcimsg("ok 5\n");
	// 6. Start link startup procedure
	WAIT_UNTIL(read_regmap_HCS_UCRDY(regmap) == 1);
	write_regmap_IS_UCCS(regmap, read_regmap_IS_UCCS(regmap));
	WAIT_UNTIL(read_regmap_IS_UCCS(regmap) == 0);
	write_regmap_UCMDARG1(regmap, 0);
	write_regmap_UCMDARG2(regmap, 0);
	write_regmap_UCMDARG3(regmap, 0);
	asm volatile("dsb ish":::"memory");
	write_regmap_UICCMD(regmap, UICCMD_CMDOP_DME_LINKSTARTUP);
	hcimsg("ok 6\n");
	asm volatile("dsb ish":::"memory");
	delay();
	WAIT_UNTIL(read_regmap_IS_UCCS(regmap) == 1);
	hcimsg("ok 7\n");
	if (read_regmap_UCMDARG2_GenericErrorCode(regmap) != UICCMD_GEC_SUCCESS) {
		hcimsg("gec: %d\n", read_regmap_UCMDARG2_GenericErrorCode(regmap));
		delay();
		goto retry6;
	}
	if (!read_regmap_HCS_DP(regmap)) {
		WAIT_UNTIL(read_regmap_IS_ULSS(regmap) == 1);
		goto retry6;
	}
	hcimsg("ok 8\n");
	// 10. Enable additional interrupts by writing IE
	// 11. Initialize UTRIACR
	// 12. Issue more UIC command
	// 13. Allocate and initialize UTP Task Management Request List

	/* Update the number of request descriptors */
	ufshost->nr_utp_tskmgmt_reqdes = MAX_NR_UTP_TSKMGMT_REQDES;
	ufshost->nr_utp_txfr_reqdes = MAX_NR_UTP_TXFR_REQDES;
	ret = read_regmap_CAP_NUTRS(regmap);
	if (ret < ufshost->nr_utp_txfr_reqdes)
		ufshost->nr_utp_txfr_reqdes = ret;
	ret = read_regmap_CAP_NUTMRS(regmap);
	if (ret < ufshost->nr_utp_tskmgmt_reqdes)
		ufshost->nr_utp_tskmgmt_reqdes = ret;

	/* 4KB memory is sufficent for the max nr of reqdes. */
	ufshost->utp_tskmgmt = malloc(PAGE_SIZE);
	fail_cond(ufshost->utp_tskmgmt == NULL, "malloc failed");
	fail_cond((u64)ufshost->utp_tskmgmt & (PAGE_SIZE - 1),
		  "allocated page is not aligned");
	/* Touch and clear the page. */
	hcimsg("ufshost->utp_tskmgmt: va=0x%lx @(0x%lx)\n",
	       ufshost->utp_tskmgmt,
	       &ufshost->utp_tskmgmt);
	memset(ufshost->utp_tskmgmt, 0, PAGE_SIZE);
	hcimsg("ufshost->utp_tskmgmt: va=0x%lx\n",
	       ufshost->utp_tskmgmt);
	for (i = 0; i < ufshost->nr_utp_tskmgmt_reqdes; ++i) {
		/* When cleared to ‘0’, hardware shall not set IS.UTMRCS to '1'
		   on completion of this command. */
		write_utp_tskmgmt_reqdes_I(&ufshost->utp_tskmgmt[i], 0);
	}
	hcimsg("ufshost->utp_tskmgmt: va=0x%lx\n",
	       ufshost->utp_tskmgmt);
	hcimsg("ok 13\n");

	// 14. Set the address of utp_tskmgmt
	hcimsg("usys_get_phys_addr: va=0x%lx\n", ufshost->utp_tskmgmt);
	ret = usys_get_phys_addr(ufshost->utp_tskmgmt, &paddr);
	fail_cond(ret < 0, "usys_get_phys_addr ret %d\n", ret);
	write_regmap_UTMRLBA(regmap, ADDR_LOWER32(paddr));
	write_regmap_UTMRLBAU(regmap, ADDR_UPPER32(paddr));
	hcimsg("ok 14\n");

	// 15. Allocate and initialize UTP Transfer Request List
	/* 4KB memory is sufficent for the max nr of reqdes. */
	ufshost->utp_txfr = malloc(PAGE_SIZE);
	fail_cond(ufshost->utp_txfr == NULL, "malloc failed");
	fail_cond((u64)ufshost->utp_txfr & (PAGE_SIZE - 1),
		  "allocated page is not aligned");
	/* Touch and clear the page. */
	memset(ufshost->utp_txfr, 0, PAGE_SIZE);
	for (i = 0; i < ufshost->nr_utp_txfr_reqdes; ++i) {
		/* Command type 1, UFS Storage */
		write_utp_txfr_reqdes_CT(&ufshost->utp_txfr[i], 1);
		/* Disable crypto */
		write_utp_txfr_reqdes_CE(&ufshost->utp_txfr[i], 0);
	}
	hcimsg("ok 15\n");

	/* Allocate UCD pages for each txfr reqdes. */
	ufshost->utp_cmds = malloc(PAGE_SIZE *
				   ufshost->nr_utp_txfr_reqdes);
	fail_cond(ufshost->utp_cmds == NULL, "malloc failed");

	// 16. Set the address of utp_txfr
	ret = usys_get_phys_addr(ufshost->utp_txfr, &paddr);
	fail_cond(ret < 0, "usys_get_phys_addr ret %d\n", ret);
	write_regmap_UTRLBA(regmap, ADDR_LOWER32(paddr));
	write_regmap_UTRLBAU(regmap, ADDR_UPPER32(paddr));
	hcimsg("ok 16\n");

	// 17. Enable utp_tskmgmt
	write_regmap_UTMRLRSR(regmap, 1);
	hcimsg("ok 17\n");

	// 18. Enable utp_txfr
	write_regmap_UTRLRSR(regmap, 1);
	hcimsg("ok 18\n");

	// 19. FIXME(MK): ??? The document is confusing.
	// write_regmap_bMaxNumOfRTT = min(bDeviceRTTCap and NORTT);
	hcimsg("ok 19\n");

	/* Enable UCCE here for future UFS commands */
	write_regmap_IE_UCCE(regmap, 1);

	/* Why do we need to add 32? */
	ufs_irq_cap = usys_irq_register(0x116 + 32);
	while (1 == usys_irq_wait(ufs_irq_cap, false)) {
		printf("Remove obsolete irq. Acking irq\n");
		usys_irq_ack(ufs_irq_cap);
	}


	void *test_buf = malloc(PAGE_SIZE * 4);
	memset(test_buf, 0, PAGE_SIZE * 4);

	hcimsg("begin test\n");

	ufs_test(ufshost);

	hcimsg("begin the 2nd test\n");

	ufs_read(ufshost, test_buf, 10);

	hcimsg("end test\n");


	return 0;
}
