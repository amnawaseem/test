#include <phidias.h>
#include <vm.h>
#include <schedule.h>
#include <memarea.h>
#include <emulate/core.h>
#include <emulate/irq.h>
#include <arch/cpu_ops.h>

static const memarea *find_archpage(const vm_cpu *vm_cpu_arch) {
	uint32_t i;
	uintptr_t archpage = (uintptr_t)vm_cpu_arch->cpu_state;

	for (i = 0; i < vm_cpu_arch->num_memareas; i++) {
		if (vm_cpu_arch->memareas[i].reference_area->vaddr == archpage) {
			return vm_cpu_arch->memareas + i;
		}
	}

	return NULL;
}

void vm_cpu_generic_hypercall(uint32_t code, uint32_t arg, uint32_t *response) {
	const memarea *archpage_memarea;

	switch (code) {
	case HYPERCALL_SLEEP:
		if (emulate_irq_activate_highest_interrupt(current_vm_cpu) == NO_PENDING_INTERRUPT) {
			// should reschedule instead if we have other VMs
			cpu_idle();
		} else {
			return;
		}
		break;
	case HYPERCALL_QUERY_ARCHPAGE:
		archpage_memarea = find_archpage(current_vm_cpu);
		if (archpage_memarea != NULL) {
			*response = archpage_memarea->vaddr;
		} else {
			*response = 0;
		}
		return;
	default:
		printf("Unknown generic hypercall %x(%x)\r\n", code, arg);
		current_sched_entity->state = SCHEDULER_STATE_ZOMBIE;
		reschedule();
	}
}
