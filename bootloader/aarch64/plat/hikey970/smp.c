#include <boot.h>
#include <printf.h>

/* Hikey970 use psci 0.2 */
#define PSCI_CPU_ON_SMC64               0xC4000003
#define PSCI_AFFINITY_INFO_SMC64	0xC4000004
#define PSCI_SUCCESS                    0
#define PSCI_NOT_SUPPORTED              -1
#define PSCI_INVALIDE_PARAMETERS        -2
#define PSCI_DENIED                     -3
#define PSCI_ALREADY_ON                 -4
#define PSCI_ON_PENDING                 -5
#define PSCI_INTERNAL_FAILURE           -6
#define PSCI_NOT_PRESENT                -7
#define PSCI_DISABLED                   -8
#define PSCI_INVALID_ADDRESS            -9

#define PSCI_AFF_INFO_ON_PENDING	2
#define PSCI_AFF_INFO_OFF		1
#define PSCI_AFF_ON			0

/* MPIDR AFF0 => core in processor, AFF1 => processor */
#define PLAT_CPU_NUM	8

unsigned long MPIDR[PLAT_CPU_NUM] = {
	0x0,			/* A53 core 0 */
	0x1,			/* A53 core 1 */
	0x2,			/* A53 core 2 */
	0x3,			/* A53 core 3 */
	0x100,			/* A73 core 0 */
	0x101,			/* A73 core 1 */
	0x102,			/* A73 core 2 */
	0x103			/* A73 core 3 */
};

void *plat_wake_up_cores(void *phy_addr)
{
	int ret = 0;
	int core_id = 0;

	for (core_id = 1; core_id < PLAT_CPU_NUM; core_id++) {
		ret =
		    boot_smc_call(PSCI_CPU_ON_SMC64, MPIDR[core_id],
				  (unsigned long)phy_addr, 0, 0, 0, 0, 0);
		if (ret != PSCI_SUCCESS) {
			printf
			    ("[PSCI] ERROR when wake core %d PSCI ret %d\r\n",
			     core_id, ret);
			HLT;
		}
		ret =
		    boot_smc_call(0xC4000004, MPIDR[core_id], 0, 0, 0, 0, 0, 0);
		while (ret != PSCI_AFF_ON) {
			if (ret == PSCI_AFF_INFO_OFF) {
				printf("[PSCI] Fail to bring cpu %d up\r\n",
				       core_id);
				HLT;
			}
			ret =
			    boot_smc_call(0xC4000004, MPIDR[core_id], 0, 0, 0,
					  0, 0, 0);
		}
	}
	printf
	    ("[PSCI] Bring all %d cpu up and start at 0x%lx\r\n",
	     core_id, phy_addr);

	return phy_addr;
}
