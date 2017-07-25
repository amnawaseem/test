#include <phidias.h>

uint32_t top_of_stack = 0;

void cpu_dcache_flush(int do_clean, int do_invalidate) {
	uint32_t clidr, ccsidr;
	uint32_t level;
	uint32_t set;
	uint32_t way;
	uint32_t set_shift;
	uint32_t highest_set, highest_way;
	uint32_t associativity_log2;

	asm volatile("dsb nsh" ::: "memory");
	asm volatile("mrc p15, 1, %0, c0, c0, 1" : "=r" (clidr));

	for (level = 0; level < (clidr >> 24); level++) {
		uint32_t cachetype = (clidr >> (3*level)) & 7;
		if (!(cachetype & 2)) continue;
		asm volatile("mcr p15, 2, %1, c0, c0, 0\n\tisb\n\t"
				"mrc p15, 1, %0, c0, c0, 0" : "=r" (ccsidr)
				: "r" (level << 1));
		set_shift = (ccsidr & 7) + 2;
		highest_way = (ccsidr >> 3) & 0x3ff;
		for (associativity_log2 = 0; highest_way; associativity_log2++) { highest_way >>= 1; }
		highest_way = (ccsidr >> 3) & 0x3ff;
		highest_set = (ccsidr >> 13) & 0x7fff;
		for (set = 0; set <= highest_set; set++) {
			for (way = 0; way <= highest_way; way++) {
				uint32_t cookie = (way << (32-associativity_log2)) | (set << set_shift);
				if (do_clean)
					asm volatile("mcr p15, 0, %0, c7, c10, 2" :: "r" (cookie));
				if (do_invalidate)
					asm volatile("mcr p15, 0, %0, c7, c6, 2" :: "r" (cookie));
			}
		}
	}
}
