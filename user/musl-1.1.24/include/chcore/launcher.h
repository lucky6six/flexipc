#pragma once

#include <chcore/elf.h>

struct new_process_caps {
	/* The cap of the main thread of the newly created process */
	int mt_cap;
};

int readelf_from_fs(const char *pathbuf, struct user_elf *user_elf);
int chcore_new_process(int argc, char *__argv[], int is_bbapplet);

int create_process(int argc, char *__argv[], struct new_process_caps *caps);

