#pragma once
#include <chcore/pmu.h>
#include <errno.h>

#define NET_BUFSIZE	4096
#define NET_SERVERPORT	9000

#define PREFIX          "[tests]"
#define EPREFIX         "[error]"

#define info(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#define error(fmt, ...) printf(EPREFIX " " fmt, ##__VA_ARGS__)

#define chcore_assert(test, message) \
	if (!(test)) { \
		error("%s failed: %s:%d: %s errno: %d\n", __func__, __FILE__, __LINE__, message, errno); \
                while (1); \
	}

static inline void delay()
{
	u64 begin, end;

	pmu_clear_cnt();
	begin = pmu_read_real_cycle();
	do {
		end = pmu_read_real_cycle();
	} while (end - begin < 500000000);
}
