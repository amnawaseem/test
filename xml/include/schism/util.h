#ifndef _SCHISM_UTIL_H_
#define _SCHISM_UTIL_H_

#define		ALIGN_MASK(shift)		((1UL << (shift)) - 1)
#define		_DO_ALIGN_UP(v, shift)		(((v) + ALIGN_MASK(shift)) & ~ALIGN_MASK(shift))
#define		_DO_ALIGN_DOWN(v, shift)	((v) & ~ALIGN_MASK(shift))

#define		_DO_CLAMP(v, low, high)		((v) < (low) ? (low) : ((v) > (high) ? (high) : (v)))

static inline uint32_t LOG2_CEIL(uint64_t val) {
	uint32_t highest_one = 63 - __builtin_clzl(val);

	if (val == (1UL << highest_one))
		return highest_one;
	else
		return highest_one + 1;
}

#define		ARRAYLEN(arr)			(sizeof(arr) / sizeof(*(arr)))

#endif
