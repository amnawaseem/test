#include <phidias.h>
#include <arch/fpu_support.h>

uint64_t top_of_stack = 0;

void cpu_dcache_flush(int do_clean, int do_invalidate) {
	// register values
	uint64_t clidr, ccsidr;
	uint64_t cachetype;
	// obtained/computed values
	uint32_t num_ways, num_sets;
	uint32_t linesize_log2, shift_ways;
	// iterators
	uint32_t level, way, set;

	asm volatile("dsb nsh" ::: "memory");
	asm volatile("mrs %0, CLIDR_EL1" : "=r" (clidr));

	// flush until LoUU (Unification Uniprocessor): [29:27]
	// flush until LoC (Coherency): [26:24]

	for (level = 0; level < BITS(clidr, 24, 3); level++) {
		// determine cache type at this level
		cachetype = BITS(clidr, (3*level), 3);
		// no cache here: then no further caches exist -> EXIT
		if (cachetype == 0)
			break;
		// ICACHE only at this level: skip
		if (cachetype == 1)
			continue;

		// select this level, then read level description
		asm volatile("msr CSSELR_EL1, %1\n\t"
				"isb\n\t"
				"mrs %0, CCSIDR_EL1"
				: "=r" (ccsidr) : "r" (level << 1));

		// get line size, associativity and number of sets
		linesize_log2 = BITS(ccsidr, 0, 3) + 4;
		num_ways = BITS(ccsidr, 3, 10) + 1;
		num_sets = BITS(ccsidr, 13, 15) + 1;

		// determine shift offset for WAYS in argument
		if (num_ways == 1)
			shift_ways = 31;
		else
			shift_ways = 31 - highest_bit(num_ways - 1);
//printf("Flush Level %d CLIDR %x CCSIDR %x (NW/SW NS: %d/%d %d)\r\n", level, clidr, ccsidr, num_ways, shift_ways, num_sets);
		for (set = 0; set < num_sets; set++) {
			for (way = 0; way < num_ways; way++) {
				uint64_t cookie = (way << shift_ways) | (set << linesize_log2) | (level << 1);
				if (do_clean)
					asm volatile("DC CSW, %0" :: "r" (cookie));
				if (do_invalidate)
					asm volatile("DC ISW, %0" :: "r" (cookie));
			}
		}
	}

	asm volatile("isb" ::: "memory");
}
