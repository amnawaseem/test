#include "map_iter.h"
#include "mmu.h"
#include "mpu.h"

enum permission_index {
	PERM_R = 0, PERM_W = 2, PERM_X = 4,
	PERM_G = 6, PERM_D = 8, PERM_U = 10, PERM_S = 12
};

struct flaggroup {
	const char *flaggroup;
	uint64_t bits_set;
	uint64_t bits_clear;
};

extern uint64_t pagetable_base;

extern int verbose_mode;

#define verbose_printf		if (verbose_mode) printf
