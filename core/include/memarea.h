#ifndef	_PHIDIAS_MEMAREA_H_
#define	_PHIDIAS_MEMAREA_H_

#include <asm/memarea.h>
#include <specification.h>

struct memarea_ {
	uintptr_t		paddr;
	uintptr_t		vaddr;
	uintptr_t		size;
	uint32_t		flags;
	const struct memarea_	*reference_area;
};
typedef struct memarea_	memarea;

struct tree_memarea_ {
	const struct tree_memarea_	*left;
	const struct tree_memarea_	*right;
	const struct memarea_		*item;
};
typedef struct tree_memarea_	tree_memarea;

const struct memarea_ *find_memarea_by_va(const struct tree_memarea_ *tree, uintptr_t vaddr);

static inline const struct memarea_ *core_memarea(uint32_t index) {
	return _specification.cpus[cpu_number].memareas + index;
}

static inline uintptr_t core_physical_address(uintptr_t virtual) {
	return (virtual - _specification.specification_va
			+ _specification.specification_pa);
}

#endif
