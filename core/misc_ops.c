#include <phidias.h>
#include <arch/cpu_ops.h>

# define	MWORD_ALIGNMENT_MASK	((WORDSIZE >> 3) - 1)

uint32_t strlen(const char *string) {
	uint32_t length = 0;

	while (*string) {
		++length;
		++string;
	}

	return length;
}

void memset(void *dest, uint8_t val, uintptr_t length) {
	uint32_t *_dest = dest;
	uint32_t _val = (val) | (val << 8) | (val << 16) | (val << 24);

	while (length > 0) {
		*(_dest++) = _val;
		length -= 4;
	}
}

void memcpy(void *dest, const void *src, uintptr_t length) {
	if (((uintptr_t)dest | (uintptr_t)src | length) & MWORD_ALIGNMENT_MASK) {
		uint8_t *_dest8 = dest;
		const uint8_t *_src8 = src;

		while (length > 0) {
			*(_dest8++) = *(_src8++);
			--length;
		}
	} else {
#if WORDSIZE == 32
		uint32_t *_dest32 = dest;
		const uint32_t *_src32 = src;

		while (length > 0) {
			*(_dest32++) = *(_src32++);
			length -= 4;
		}
#elif WORDSIZE == 64
		uint64_t *_dest64 = dest;
		const uint64_t *_src64 = src;

		while (length > 0) {
			*(_dest64++) = *(_src64++);
			length -= 8;
		}
#else
# error Unsupported word size.
#endif
	}
}

void __attribute__((noreturn)) panic() {
	while (1) {
		cpu_zombie();
	}
}
