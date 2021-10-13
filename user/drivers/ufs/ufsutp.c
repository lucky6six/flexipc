/*-
 * UFS Transport Protocol (UTP) Layer.
 *
 * Terminologies:
 * UPIU: UFS Protocol Information Units, which seems like a universial format
 * for messages.
 * LUN: Logical Unit Number
 * CDB: Command Descriptor Blocks
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <syscall.h>
#include <string.h>
#include <chcore/bug.h>
#include <chcore/ipc.h>
#include <chcore/proc.h>
#include <chcore/defs.h>
#include <bits/errno.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint64_t paddr_t;
#define ADDR_LOWER32(addr) ((addr) & 0xffffffff)
#define ADDR_UPPER32(addr) ADDR_LOWER32((addr) >> 32)

#include "ufs-bitfields.gen.h"


#define UPIU_READ (0)
#define UPIU_WRITE (1)
int utp_build_rw_command_upiu(void *upiu, int rw, u32 len)
{
	write_upiu_header_TxnType(upiu, UPIU_TXN_COMMAND);
	/* TODO(MK): Consider to add CRC later? */
	write_upiu_header_TxnType_DD(upiu, 0);
	write_upiu_header_TxnType_HD(upiu, 0);

	switch (rw) {
	case UPIU_READ:
		write_upiu_header_Flags_R(upiu, 1);
		write_upiu_header_Flags_W(upiu, 0);
		break;
	case UPIU_WRITE:
		write_upiu_header_Flags_R(upiu, 0);
		write_upiu_header_Flags_W(upiu, 1);
		break;
	default:
		printf("error read write\n");
	}
	/* TODO(MK): Support more than simple */
	write_upiu_header_Flags_ATTR(upiu, UPIU_TSKATTR_SIMPLE);
	/* TODO(MK): Support priority */
	write_upiu_header_Flags_CP(upiu, UPIU_CP_NO_PRIORITY);
	/* Command UPIU does not txfr/recv data directly */
	write_upiu_header_DataSegLen(upiu, 0);
	/* TODO(MK): Build an IID mgmt */
	write_upiu_header_IID(upiu, 0);
	write_upiu_header_CmdSet(upiu, 0);

	return 0;
}

int utp_build_command_upiu_read(void *upiu, int block_nr)
{
	memset(upiu, 0, 32);
	write_upiu_header_TxnType(upiu, UPIU_TXN_COMMAND);
	/* TODO(MK): Consider add CRC later? */
	write_upiu_header_TxnType_DD(upiu, 0);
	write_upiu_header_TxnType_HD(upiu, 0);

	/* Read */
	write_upiu_header_Flags_R(upiu, 1);
	write_upiu_header_Flags_W(upiu, 0);
	/* TODO(MK): Support more than simple */
	write_upiu_header_Flags_ATTR(upiu, UPIU_TSKATTR_SIMPLE);
	/* TODO(MK): Support priority */
	write_upiu_header_Flags_CP(upiu, UPIU_CP_NO_PRIORITY);
	/* Command UPIU does not txfr/recv data directly */
	write_upiu_header_DataSegLen(upiu, 0);
	/* TODO(MK): Build an IID mgmt */
	write_upiu_header_IID(upiu, 0);
	write_upiu_header_CmdSet(upiu, 0);

	/* FIXME(MK): Hardcoded for testing */
	/* Data transfer length: 4K */
	(*(u32 *)(upiu + 12)) = htobe32(4096);
	u64 lba = block_nr;
	u8 nr_blocks = 1;
	u8 *cmd = upiu + 16;
	/* Refer to doc JESD220C-UFS2.1 $11.3.5 */
	cmd[0] = 0x08; /* Read(6) Command */
	cmd[3] = lba & 0xFF;
	cmd[2] = (lba >> 8) & 0xFF;
	cmd[1] = (lba >> 16) & 0xFF;
	/* (nr_blocks == 0) => read 256 blocks */
	BUG_ON(nr_blocks > 255);
	cmd[4] = nr_blocks;
	cmd[5] = 0x00;

	return 0;
}

int utp_build_response_upiu(void *upiu)
{
	return 0;
}
