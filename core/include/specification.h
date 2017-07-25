#ifndef	_PHIDIAS_SPECIFICATION_H_
#define	_PHIDIAS_SPECIFICATION_H_

extern uint32_t cpu_number;

struct memarea_;
struct tree_memarea_;
struct specification_cpu_;
struct specification_arch_;
struct vm_;
struct vm_cpu_;

struct specification_ {
	char		magic[8];
	uintptr_t	specification_pa;
	uintptr_t	specification_va;
	uintptr_t	boot_pagetable_address;
	uint32_t	physical_cpu_count;
	const struct specification_cpu_	*cpus;
	const struct vm_ * const *irq_owner_table;
	const struct specification_arch_	*arch_specification;
	char		timestamp[24];
};
typedef struct specification_	specification;

extern const specification	_specification;

struct specification_cpu_ {
	uintptr_t			pagetable_address;

	uint32_t			num_memareas;
	const struct memarea_		*memareas;
	const struct tree_memarea_	*memarea_tree;

	uint32_t			num_vm_cpus;
	struct vm_cpu_ * const		*vm_cpus;

	uint32_t			__pad[2];
};
typedef struct specification_cpu_	specification_cpu;

#endif
