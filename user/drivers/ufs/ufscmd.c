/**
 * UFS Commands (SCSI Commands) according to JEDEC STANDARD UFS V2.1 JESD220C.
 */

#include <chcore/type.h>

#define UFSCMD_FORMAT_UNIT   (0x04)
#define UFSCMD_INQUIRY       (0x12)
#define UFSCMD_MODE_SELECT10 (0x55)
#define UFSCMD_MODE_SENSE10  (0x5A)
#define UFSCMD_PRE_FETCH10   (0x34)
#define UFSCMD_PRE_FETCH16   (0x90) /* Optional */
#define UFSCMD_READ6         (0x08)
#define UFSCMD_READ10        (0x28)
#define UFSCMD_READ16        (0x88) /* Optional */
#define UFSCMD_READ_BUFFER    (0x3C) /* Optional */
#define UFSCMD_READ_CAPACITY10 (0x25)
#define UFSCMD_READ_CAPACITY16 (0x9E)
#define UFSCMD_REPORT_LUNS      (0xA0)
#define UFSCMD_REQUEST_SENSE    (0x03)
#define UFSCMD_SECURITY_PROTO_IN (0xA2)
#define UFSCMD_SECURITY_PROTO_OUT (0xB5)
#define UFSCMD_SEND_DIAGNOSTIC (0x1D)
#define UFSCMD_START_STOP_UNIT (0x1B)
#define UFSCMD_SYNC_CACHE10 (0x35)
#define UFSCMD_SYNC_CACHE16 (0x91) /* Optional */
#define UFSCMD_TEST_UNIT_READY (0x00)
#define UFSCMD_UNMAP     (0x42)
#define UFSCMD_VERIFY10 (0x2F)
#define UFSCMD_WRITE6   (0x0A)
#define UFSCMD_WRITE10  (0x2A)
#define UFSCMD_WRITE16  (0x8A) /* Optional */
#define UFSCMD_WRITE_BUFFER (0x3B)



/**
 * Read from UFS.
 * @lba: the address of the first block;
 * @nr_blocks: number of blocks to read. 256 blocks are read if nr_blocks
 * is 0.
 * Returns the number of bytes filled.
 */
u32 ufs_make_read6(void *buf, u64 lba, u8 nr_blocks)
{
	u8 *cmd = buf;
	cmd[0] = UFSCMD_READ6;
	cmd[3] = lba & 0xFF;
	cmd[2] = (lba >> 8) & 0xFF;
	cmd[1] = (lba >> 16) & 0xFF;
	cmd[4] = nr_blocks;
	cmd[5] = 0x00;
	return 6;
}

/**
 * Read from UFS, 10-byte format.
 * @lba: the address of the first block;
 * @nr_blocks: number of blocks to read. No blocks are read if nr_blocks is 0;
 */
#define UFS_READ10_DPO_BIT (1 << 4) /* Disable Page Out: Lowest rention priority when set */
#define UFS_READ10_FUA_BIT (1 << 3) /* Force Unit Access: Do not read from cache when set */
void ufs_read10(u64 lba, int retain_page, int force_access, u8 group_number,
		u16 nr_blocks)
{
	u8 cmd[10];
	cmd[0] = UFSCMD_READ10;
	cmd[1] = (retain_page ? UFS_READ10_DPO_BIT : 0) ||
		(force_access ? UFS_READ10_FUA_BIT : 0);
	cmd[5] = lba & 0xFF;
	cmd[4] = (lba >> 8) & 0xFF;
	cmd[3] = (lba >> 16) & 0xFF;
	cmd[2] = (lba >> 24) & 0xFF;
	cmd[6] = group_number;
	cmd[8] = nr_blocks & 0xFF;
	cmd[7] = (nr_blocks >> 8) & 0xFF;
	cmd[9] = 0x00;
}

/**
 * Write to UFS.
 * @lba: the address of the first block;
 * @nr_blocks: number of blocks to write. 256 blocks are written if nr_blocks
 * is 0.
 * Returns the number of bytes filled.
 */
u32 ufs_make_write6(void *buf, u64 lba, u8 nr_blocks)
{
	u8 *cmd = buf;
	cmd[0] = UFSCMD_WRITE6;
	cmd[3] = lba & 0xFF;
	cmd[2] = (lba >> 8) & 0xFF;
	cmd[1] = (lba >> 16) & 0xFF;
	cmd[4] = nr_blocks;
	cmd[5] = 0x00;
	return 6;
}

/**
 * Write to UFS, 10-byte format.
 * @lba: the address of the first block;
 * @nr_blocks: number of blocks to write. 256 blocks are written if nr_blocks
 * is 0.
 */
void ufs_write10(u64 lba, u8 nr_blocks)
{
	u8 cmd[6];
	cmd[0] = UFSCMD_WRITE10;
	cmd[3] = lba & 0xFF;
	cmd[2] = (lba >> 8) & 0xFF;
	cmd[1] = (lba >> 16) & 0xFF;
	cmd[4] = nr_blocks;
	cmd[5] = 0x00;
}
