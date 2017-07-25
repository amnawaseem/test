#include <phidias.h>
#include <misc_ops.h>
#include <interrupts.h>
#include <drivers/irq.h>
#include <xcore.h>
#include <vm.h>
#include <trace.h>
#include <schedule.h>
#include <arch/cpu_ops.h>

static const xcore_buffer *xcore_get_input_buffer(uint32_t other_cpu) {
	return (const xcore_buffer *)core_memarea(MEMAREA_XCORE)->vaddr + (8*other_cpu) + cpu_number;
}

static xcore_buffer *xcore_get_output_buffer(uint32_t other_cpu) {
	return (xcore_buffer *)core_memarea(MEMAREA_XCORE)->vaddr + (8*cpu_number) + other_cpu;
}

//////////////////////////////////////////////////////////////////////////////

/* check the number of readable words in b */
static uint32_t xcore_check_buffer_avail(const xcore_buffer *b, xcore_buffer *b_reverse) {
	uint32_t rp = b_reverse->reverse_read_offset;
	uint32_t wp = b->write_offset;

	if (rp > wp) wp += XCORE_BUFFER_SIZE;
	return wp - rp;
}

static wordsize_t xcore_pull_buffer(const xcore_buffer *b, xcore_buffer *b_reverse) {
	wordsize_t m = b->buffer[b_reverse->reverse_read_offset];
	b_reverse->reverse_read_offset = (b_reverse->reverse_read_offset + 1) % XCORE_BUFFER_SIZE;
	return m;
}

//////////////////////////////////////////////////////////////////////////////

/* check whether another word can be written into b */
static uint32_t xcore_check_buffer_room(xcore_buffer *b, const xcore_buffer *b_reverse, uint32_t count) {
	uint32_t rp = b_reverse->reverse_read_offset;
	uint32_t wp = b->write_offset;

	if (rp <= wp) rp += XCORE_BUFFER_SIZE;
	return (wp + count < rp);
}

static void xcore_put_buffer(xcore_buffer *b, uint32_t off, wordsize_t word) {
	uint32_t b_off = (b->write_offset + off) % XCORE_BUFFER_SIZE;
	b->buffer[b_off] = word;
}

static void xcore_advance_buffer(xcore_buffer *b, uint32_t count) {
	b->write_offset = (b->write_offset + count) % XCORE_BUFFER_SIZE;
}

//////////////////////////////////////////////////////////////////////////////

/**
 * Forwards a virtual IRQ <vector> sent from another VM to vm:to.
 */
void xcore_raise_interrupt_remote(const vm *vm, uint32_t vector) {
	uint32_t target_pcpu = vm->cpus[0].physical_cpu;
	xcore_buffer *b = xcore_get_output_buffer(target_pcpu);
	const xcore_buffer *br = xcore_get_input_buffer(target_pcpu);

	if (xcore_check_buffer_room(b, br, 3)) {
		xcore_put_buffer(b, 0, XCOREMESSAGE_REMOTE_IRQ);
		xcore_put_buffer(b, 1, (uintptr_t)vm);
		xcore_put_buffer(b, 2, vector);
		xcore_advance_buffer(b, 3);
	}

	arch_barrier();
	irq_raise_ipi(target_pcpu);
}

/**
 * Forwards a vIPI <vector> from vm:from to vm:to.
 */
void xcore_virtual_ipi(const vm *vm, uint32_t from_vcpu, uint32_t to_vcpu, uint32_t vector) {
	uint32_t target_pcpu = vm->cpus[to_vcpu].physical_cpu;
	xcore_buffer *b = xcore_get_output_buffer(target_pcpu);
	const xcore_buffer *br = xcore_get_input_buffer(target_pcpu);

	if (xcore_check_buffer_room(b, br, 5)) {
		xcore_put_buffer(b, 0, XCOREMESSAGE_VIPI);
		xcore_put_buffer(b, 1, (uintptr_t)vm);
		xcore_put_buffer(b, 2, from_vcpu);
		xcore_put_buffer(b, 3, to_vcpu);
		xcore_put_buffer(b, 4, vector);
		xcore_advance_buffer(b, 5);
	}

	//printf("vIPI %d -> %d\r\n", cpu_number, target_pcpu);
	arch_barrier();
	irq_raise_ipi(target_pcpu);
}

//////////////////////////////////////////////////////////////////////////////

static void handle_ipi(const xcore_buffer *b_in, xcore_buffer *b_rev) {
	wordsize_t message = xcore_pull_buffer(b_in, b_rev);
	const vm *target_vm;
	uint32_t sender_vcpu, target_vcpu;
	uint32_t vector;

	/*printf("IPI(%d): %x (%d--%d)\r\n", cpu_number, message,
		b_rev->reverse_read_offset, b_in->write_offset);
	printf("Q Ptr: %qx %qx\r\n", (uintptr_t)b_in, (uintptr_t)b_rev);*/

	switch (message) {
	case XCOREMESSAGE_VIPI:
		target_vm = (const vm *)xcore_pull_buffer(b_in, b_rev);
		sender_vcpu = xcore_pull_buffer(b_in, b_rev);
		target_vcpu = xcore_pull_buffer(b_in, b_rev);
		vector = xcore_pull_buffer(b_in, b_rev);
		deliver_vm_ipi(target_vm, target_vcpu, vector); /* sender_vcpu */
		(void)sender_vcpu;
		break;
	case XCOREMESSAGE_REMOTE_IRQ:
		target_vm = (const vm *)xcore_pull_buffer(b_in, b_rev);
		vector = xcore_pull_buffer(b_in, b_rev);
		if (vector == 2) {
			uint32_t j;
			for (j = 0; j < XCORE_BUFFER_SIZE; j++)
				printf("%c[%d] %qx\r\n", (j == b_rev->reverse_read_offset) ? 'R' :
							(j == b_in->write_offset) ? 'W' : ' ', j,
					b_in->buffer[j]);
			panic();
		}
		deliver_irq_to_vm(target_vm, vector, 0);
		break;
	}
}

/* IPI handler */
uint32_t xcore_ipi_interrupt() {
	uint32_t i;

	//printf("IPI arrived (%d)\r\n", cpu_number);
	for (i = 0; i < 8; i++) {
		const xcore_buffer *b = xcore_get_input_buffer(i);
		xcore_buffer *br = xcore_get_output_buffer(i);

		while (xcore_check_buffer_avail(b, br)) {
			handle_ipi(b, br);
		}
	}

	return 0;
}

void xcore_setup() {
	uint32_t i;

	if (cpu_number == 0) memset((void*)core_memarea(MEMAREA_XCORE)->vaddr, 0, 4096*8*8);
	return;

	for (i = 0; i < 8; i++) {
		xcore_buffer *b = xcore_get_output_buffer(i);
		memset((void *)b, 0, 4096);
		//b->reverse_read_offset = 0;
		//b->write_offset = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////

/**
 * Capability invocations:
 * 1) cross-guest vIRQ (target guest, target IRQ)
 * 2) management calls?
 *   2a) guest (vCPU) reboot
 *   2b) PSCI-like vCPU power control
 *   2c) debugging / tracing?
 */
void capability_invoke(uint32_t cap_number) {
	const vm *cap_vm = current_vm_cpu->vm;
	const capability *cap;
	uint32_t direct_switch = 0;

	if (cap_number >= cap_vm->num_capabilities) {
		printf("Cap call ignored, invalid index\r\n");
		return;
	}

	cap = &cap_vm->capabilities[cap_number];
	// trace("cap", (uintptr_t)current_vm_cpu, cap_number, cap->capability_type, cap->capability_param);

	switch (cap->capability_type) {
	case CAPABILITY_TYPE_IRQ:
		if (cap->target->cpus[0].physical_cpu != cpu_number) {
			xcore_raise_interrupt_remote(cap->target, cap->capability_param);
			return;
		}
		if (cap->target->cpus[0].sched_entities[0].state != SCHEDULER_STATE_READY)
			direct_switch = 1;
		deliver_irq_to_vm(cap->target, cap->capability_param, 0);
		//TEST-ONLY// xcore_virtual_ipi((const vm *)0xf400db08, 0, 0, 4);
		if (direct_switch) {
			reschedule();
		}
		break;
	default:
		printf("Unknown capability type\r\n");
		panic();
	}
}
