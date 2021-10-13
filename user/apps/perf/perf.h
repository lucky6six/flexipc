#pragma once

#define PREFIX          "[perf]"
#define EPREFIX         "[error]"

#define info(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#define error(fmt, ...) printf(EPREFIX " " fmt, ##__VA_ARGS__)
