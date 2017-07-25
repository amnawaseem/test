#include <phidias.h>
#include <schedule.h>
#include <vm.h>
#include <emulate/core.h>
#include <emulate/vtlb.h>
#include <trace.h>
#include <arch/vm.h>

static void vm_cpu_fault_3(uintptr_t intermediate_address, uint32_t flags) {
	const emulate *emul;

	emul = find_emulate_by_va(current_vm_cpu->emulate_tree, intermediate_address);

	if (emul != NULL) {
		// printf("(SP %x) Emul Guest Fault: [%x/%x/??]\r\n", (uintptr_t)&address, walked_address, flags);
		vm_cpu_emulate_faulted(current_vm_cpu, emul, intermediate_address);
		upcall();
	} else {
		printf("UNKNOWN Guest Fault: [%x/%x/??]\r\n", intermediate_address, flags);
	}
	vm_cpu_dump_state(current_vm_cpu);
	sched_state_modify(current_sched_entity, SCHEDULER_STATE_ZOMBIE);
	/* leads to a reschedule() */
}

#ifdef EMULATE_VTLB
static void vm_cpu_fault_1_2(uintptr_t address, uint32_t flags, uint32_t hw_flags) {
	uintptr_t walked_address;
	uintptr_t mapping_base = 0;
	uintptr_t mapping_size = 0;
	uint32_t mapping_flags = 0;
	uint32_t extra_flags;
	const memarea *area;

	trace("f", ((uint32_t *)current_vm_cpu->cpu_state)[17], ((uint32_t *)current_vm_cpu->cpu_state)[16],
		address, hw_flags);
	walked_address = emulate_vtlb_walk(current_vm_cpu, address, &mapping_base, &mapping_size, &mapping_flags);

	if (mapping_size == 0) {
//
		printf("Walk Fail: [%x] -> [%x], covered by [%x/%x/%x] (hw %x), wanted %x\r\n",
			address, walked_address,
			mapping_base, mapping_size, mapping_flags, hw_flags, flags);
//
		vm_cpu_inject_fault(current_vm_cpu, address, flags);
		upcall();
	}

	if ((flags & mapping_flags) < flags) {
//
		printf("Walk Perm: [%x] -> [%x], covered by [%x/%x/%x] (hw %x), wanted %x\r\n",
			address, walked_address,
			mapping_base, mapping_size, mapping_flags, hw_flags, flags);
//
		vm_cpu_inject_fault(current_vm_cpu, address, flags);
		upcall();
	}

	area = find_memarea_by_va(current_vm_cpu->memarea_tree, walked_address);

	if (area != NULL) {
		extra_flags = (current_vm_cpu->vtlb_emulate->control.vtlb->is_paravirt) ? MEMAREA_FLAG_U : 0;
//
		printf("Vtlb Guest Fault: [%x/%x/%x]\r\n", walked_address, flags, hw_flags);
		printf("Adding map: [%x+%x] -> [%x] F %x\r\n",
				mapping_base + address - walked_address,
				mapping_size,
				mapping_base - area->vaddr + area->paddr,
				(mapping_flags & area->flags) | extra_flags);
//
		emulate_vtlb_map_range(current_vm_cpu,
				mapping_base + address - walked_address,
				mapping_base - area->vaddr + area->paddr,
				mapping_size,
				(mapping_flags & area->flags) | extra_flags);
		upcall();
	}

	vm_cpu_fault_3(walked_address, flags);
}
#endif

/**
 * This is the architecture-independent handler for page faults caused by a
 * VM. The handling of a VM page fault requires two steps:
 *
 * 1) resolving the faulting VA to a guest-physical address; if this step
 *    fails, the fault has to be injected into the VM
 * 2) checking whether the guest-physical address is a valid access
 *    (memory range assigned to the VM); if it is, create a mapping
 * 3) checking whether the guest-physical address is an emulatable access;
 *    if it is, emulate
 *
 * VM scenarios with Paravirtualization or Register-only Virtualization
 * require all three.
 * VM scenarios with Nested Paging require only 3).
 */
void vm_cpu_fault(uintptr_t address, uint32_t flags, uint32_t hw_flags) {
#ifdef EMULATE_VTLB
	vm_cpu_fault_1_2(address, flags, hw_flags);
#else
	(void)address; (void)flags; (void)hw_flags;

	printf("Paravirt fault, but no VTLB unit\r\n");
	sched_state_modify(current_sched_entity, SCHEDULER_STATE_ZOMBIE);
#endif
}

void vm_cpu_fault_nested(uintptr_t intermediate_address, uint32_t flags) {
	vm_cpu_fault_3(intermediate_address, flags);
}
