#ifndef	_PHIDIAS_VM_H_
#define	_PHIDIAS_VM_H_

#include <asm/vm.h>

struct memarea_;
struct tree_memarea_;
struct capability_;
struct emulate_;
struct tree_emulate_;

/** provided by architecture **/
struct vm_cpu_state_;

struct capability_ {
	const struct vm_		*target;
	uint32_t			capability_type;
	uint32_t			capability_param;
};
typedef struct capability_	capability;

struct vm_copyin_ {
	uintptr_t			dest;
	uintptr_t			source;
	uintptr_t			size;
};
typedef struct vm_copyin_	vm_copyin;

struct vm_ {
	uint32_t			num_cpus;
	const struct vm_cpu_		*cpus;

	uint32_t			num_capabilities;
	const struct capability_	*capabilities;

	uint32_t			num_copyins;
	const struct vm_copyin_		*copyins;

	uintptr_t			boot_address;
};
typedef struct vm_	vm;

struct vm_cpu_ {
	const struct vm_		*vm;
	struct vm_cpu_state_		*cpu_state;
	uint32_t			physical_cpu;

	uintptr_t			pagetable_base;
	uint32_t			pagetable_identifier;

	uint32_t			num_memareas;
	const struct memarea_		*memareas;
	const struct tree_memarea_	*memarea_tree;

	uint32_t			num_emulates;
	const struct emulate_		*emulates;
	const struct tree_emulate_	*emulate_tree;
	uint32_t			num_hw_emulates;
	const struct emulate_ * const	*hw_emulates;

	const struct emulate_		*vtlb_emulate;
	const struct emulate_		*irq_emulate;
	const struct emulate_		*uart_emulate;

	uint32_t			num_sched_entities;
	struct scheduler_entity_	*sched_entities;
};
typedef struct vm_cpu_	vm_cpu;

extern void vm_cpu_generic_hypercall(uint32_t code, uint32_t arg, uint32_t *response);
extern void vm_cpu_fault(uintptr_t, uint32_t, uint32_t);
extern void vm_cpu_fault_nested(uintptr_t, uint32_t);

#endif
