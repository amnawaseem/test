#include <phidias.h>
#include <vm.h>
#include <arch/vm.h>
#include <arch/cpu_state.h>
#include <arch/cpu_ops.h>
#include <arch/cp0_regs.h>

void vm_cpu_initialize(const vm_cpu *new_vm_cpu) {
	new_vm_cpu->cpu_state->pc = new_vm_cpu->vm->boot_address;
	/* set guest.HSS = 1, guest.ESS = 1 */
	new_vm_cpu->cpu_state->cp0_srsctl = (1 << 26) | (1 << 12);
	/* set guest lowest to 2 */
	new_vm_cpu->cpu_state->hcp0_guestctl3 = 2;
	/* TODO: remove magic numbers */
	new_vm_cpu->cpu_state->cp0_status = 0x31400104;
}

void vm_cpu_dump_state(const vm_cpu *vm_cpu_x) {
	vm_cpu_state *state = vm_cpu_x->cpu_state;
	printf("PC(state): %08x\tPC(EPC): %08x\r\n", state->pc, mfc0(c0_epc));
	printf("$0: %08x\tat: %08x\tv0: %08x\tv1: %08x\r\n", rfpss(0), rfpss(1), rfpss(2), rfpss(3));
	printf("a0: %08x\ta1: %08x\ta2: %08x\ta3: %08x\r\n", rfpss(4), rfpss(5), rfpss(6), rfpss(7));
	printf("t0: %08x\tt1: %08x\tt2: %08x\tt3: %08x\r\n", rfpss(8), rfpss(9), rfpss(10), rfpss(11));
	printf("t4: %08x\tt5: %08x\tt6: %08x\tt7: %08x\r\n", rfpss(12), rfpss(13), rfpss(14), rfpss(15));
	printf("s0: %08x\ts1: %08x\ts2: %08x\ts3: %08x\r\n", rfpss(16), rfpss(17), rfpss(18), rfpss(19));
	printf("s4: %08x\ts5: %08x\ts6: %08x\ts7: %08x\r\n", rfpss(20), rfpss(21), rfpss(22), rfpss(23));
	printf("t8: %08x\tt9: %08x\tk0: %08x\tk1: %08x\r\n", rfpss(24), rfpss(25), rfpss(26), rfpss(27));
	printf("gp: %08x\tsp: %08x\tfp: %08x\tra: %08x\r\n", rfpss(28), rfpss(29), rfpss(30), rfpss(31));
}

void vm_cpu_inject_supervisorcall(const vm_cpu *vm_cpu_inject) {
	/* stub for now */
	(void)*vm_cpu_inject;
}

void vm_cpu_inject_fault(const vm_cpu *vm_cpu_inject, uintptr_t address, uint32_t flags) {
	/* stub for now */
	(void)*vm_cpu_inject;
	(void)address;
	(void)flags;
}

void vm_cpu_inject_interrupt(const vm_cpu *vm_cpu_inject, uint32_t interrupt_number) {
	/* stub for now */
	(void)*vm_cpu_inject;
	(void)interrupt_number;
}

void vm_cpu_inject_undefined(const vm_cpu *vm_cpu_inject) {
	/* stub for now */
	(void)*vm_cpu_inject;
}

void vm_cpu_switch_bank(vm_cpu_state *state, uint32_t old_mode, uint32_t new_mode) {
	/* stub for now */
	(void)*state;
	(void)old_mode;
	(void)new_mode;
}

void vm_cpu_enter_vector(vm_cpu_state *state, uint32_t vector_offset) {
	/* stub for now */
	(void)*state;
	(void)vector_offset;
}
