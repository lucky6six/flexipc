#pragma once

#include <chcore/type.h>

int create_thread(void *(*func)(void *), u64 arg, u32 prio, u64 tls);
