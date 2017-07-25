#include <phidias.h>
#include <interrupts.h>
#include <vm.h>
#include <misc_ops.h>
#include <schedule.h>
#include <drivers/irq.h>
#include <drivers/timer.h>
#include <drivers/uart.h>
#include <emulate/core.h>
#include <emulate/irq.h>
#include <trace.h>

void interrupt_handler() {
	uint32_t interrupt_number_raw;
	uint32_t interrupt_number;
	const vm *owner_vm;

	interrupt_number_raw = irq_get_irq_raw();
	if (0) {
	  uint64_t *ptr = (uint64_t *)&interrupt_number_raw;
	  uint32_t i;
	  printf("Inbound IRQ %x\r\n", interrupt_number_raw);
	  for (i = 0; i < 8; i++) printf("Stk[%d]: %x\r\n", i, ptr[i]);
	}

	if (interrupt_number_raw == IRQID_SPURIOUS)
		upcall();

	interrupt_number = irq_get_irq(interrupt_number_raw);
/*
	if (current_vm_cpu) {
		printf("CPU #%d, IRQ 0x%x\r\n", cpu_number, interrupt_number);
		printf("VMState: %x:%x\r\n", ((uint64_t *)current_vm_cpu->cpu_state)[32],
					((uint64_t *)current_vm_cpu->cpu_state)[33]);
	}

	trace("i", current_vm_cpu ? ((uint32_t *)current_vm_cpu->cpu_state)[17] : ~0U,
			current_vm_cpu ? ((uint32_t *)current_vm_cpu->cpu_state)[16] : ~0U,
			current_vm_cpu ? ((uint32_t *)current_vm_cpu->cpu_state)[18] : ~0U,
			interrupt_number);

*/
	if (interrupt_number >= IRQID_MAXIMUM) {
		printf("IRQ %x is beyond HW bounds - violation!\r\n", interrupt_number_raw);
		panic();
	}

	owner_vm = _specification.irq_owner_table[interrupt_number];
	if (owner_vm != NULL) {
		deliver_irq_to_vm(owner_vm, interrupt_number, 1);
		// will be acked (reactivated) later by emulated IRQ write
		// or by hardware virtualization assistance

	} else if (irqhandlers[interrupt_number] != NULL) {
		uint32_t ret = irqhandlers[interrupt_number]();

		if (!(ret & IRQ_KEEP_MASKED))
			irq_ack_irq(interrupt_number_raw);

	} else {
		printf("Unhandled IRQ #%d, disabling\r\n", interrupt_number_raw);
		irq_disable_irq(interrupt_number);
		irq_ack_irq(interrupt_number_raw);
	}

	upcall();
}

void deliver_vm_ipi(const vm *target_vm, uint32_t vcpu_no, uint32_t interrupt_number) {
	emulate_irq_set_interrupt_pending(target_vm->cpus+vcpu_no, interrupt_number);
	sched_state_modify(target_vm->cpus[vcpu_no].sched_entities + 0, SCHEDULER_STATE_READY);
}

void deliver_irq_to_vm(const vm *target_vm, uint32_t interrupt_number, uint32_t is_passthrough) {
	// if (interrupt_number != 0xef) printf("IRQ 0x%x to VM %p\r\n", interrupt_number, target_vm);

	// TODO: check that the target VCPU is actually local to this physical CPU
	// TODO: always inject into the first VCPU of a guest?
	(void)is_passthrough;

	emulate_irq_set_interrupt_pending(target_vm->cpus+0, interrupt_number);

	sched_state_modify(target_vm->cpus[0].sched_entities + 0, SCHEDULER_STATE_READY);
}
