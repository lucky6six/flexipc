#pragma once

#include <chcore/container/hashtable.h>
#include <chcore/type.h>
#include <malloc.h>
#include <sys/types.h>

#include "defs.h"
#include "util.h"

struct openat_callback_args {
	int flags;
	mode_t mode;
};