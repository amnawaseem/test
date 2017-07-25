#ifndef	_PHIDIAS_EMULATE_VTLB_H_
#define	_PHIDIAS_EMULATE_VTLB_H_

#include <memarea.h>

# define	VTLB_PAGING_FORMAT_ARM_SHORT		0x00010001
# define	VTLB_PAGING_FORMAT_ARM_LONG		0x00010002
# define	VTLB_PAGING_FORMAT_ARM_LONG_NESTED	0x00010003

# define	VTLB_PAGING_FORMAT_X86_LEGACY		0x00020001
# define	VTLB_PAGING_FORMAT_X86_LONG		0x00020002

# define	VTLB_PAGING_FORMAT_MIPS			0x00030001

# define	VTLB_PAGING_FORMAT_ARM64_G4IO40		0x00040001

struct emulate_vtlb_pool_ {
	uint32_t			dirsize_log2;
	uint32_t			entrysize_log2;
	uint32_t			count;
	uint32_t			*bitmap;
	uint32_t			shift;

	uintptr_t			virtual_address;
	uintptr_t			physical_address;
};
typedef struct emulate_vtlb_pool_	emulate_vtlb_pool;

# define	VTLB_INVALID_PAGETABLE		(~(uintptr_t)0)
# define	VTLB_INVALID_IDENTIFIER		(~0U)
# define	VTLB_ALLOCATOR_EMPTY		(~(uintptr_t)0)

# define	VTLB_NO_ACTIVE_INSTANCE		(~0U)

struct emulate_vtlb_instance_ {
	uint64_t			last_usage;

	uint32_t			identifier;
	uintptr_t			pagetable_address;

	uint32_t			vm_privilege_level;

	uint32_t			vm_identifier;
	uintptr_t			vm_pagetable_address;

	uint32_t			__pad[1];
};
typedef struct emulate_vtlb_instance_	emulate_vtlb_instance;

struct emulate_vtlb_ {
	uint32_t			paging_format;
	uint32_t			is_paravirt;

	uint32_t			num_instances;
	struct emulate_vtlb_instance_	*instances;

	uint32_t			num_levels;
	const struct emulate_vtlb_pool_	*level_pools;

	uint32_t			active_instance;
};
typedef struct emulate_vtlb_		emulate_vtlb;

extern void emulate_vtlb_initialize(const struct vm_cpu_ *vm_cpu_reset, emulate_vtlb *vtlb);

extern void emulate_vtlb_map_range(const struct vm_cpu_ *vm_cpu_map,
		uintptr_t vaddr, uintptr_t paddr, uintptr_t size, uint32_t flags);

extern void emulate_vtlb_flush_current(const struct vm_cpu_ *vm_cpu_flush);
extern void emulate_vtlb_flush_by_identifier(const struct vm_cpu_ *vm_cpu_flush, uint32_t vm_identifier);
extern void emulate_vtlb_flush_all(const struct vm_cpu_ *vm_cpu_flush);

extern void emulate_vtlb_unmap_by_identifier(const struct vm_cpu_ *vm_cpu_flush, uint32_t vm_identifier, uintptr_t address);

extern void emulate_vtlb_switch(const struct vm_cpu_ *vm_cpu_switch, uintptr_t vm_pagetable_address, uint32_t vm_identifier);

#ifdef FEATURE_VTLB_ASSERTIONS
extern void emulate_vtlb_assert_bounds(emulate_vtlb *vtlb, uint32_t level, uintptr_t address);
#else
# define emulate_vtlb_assert_bounds(vtlb, level, address)
#endif

extern void emulate_vtlb_arch_map_range_slot(emulate_vtlb *vtlb, uint32_t i,
		uintptr_t vaddr, uintptr_t paddr, uintptr_t size, uint32_t flags);
extern void emulate_vtlb_arch_flush_slot(emulate_vtlb *vtlb, uint32_t i);
extern void emulate_vtlb_arch_unmap_slot(emulate_vtlb *vtlb, uint32_t i, uintptr_t address);

#endif
