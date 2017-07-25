#include <phidias.h>
#include <vm.h>
#include <misc_ops.h>
#include <schedule.h>
#include <emulate/core.h>
#include <emulate/vtlb.h>
#include <arch/cp15_regs.h>
#include <arch/cpu_state.h>
#include <arch/cpu_ops.h>
#include <arch/vm.h>
#include <trace.h>

static inline uint32_t get_faulting_instruction(vm_cpu_state *state) {
	return *(uint32_t *)state->pc;
}

static void emulate_thumb(const vm_cpu *vm_cpu_em, const emulate *emul, uintptr_t fault_address) {
	vm_cpu_state *state = current_vm_cpu->cpu_state;
	uint32_t insn;

	(void)vm_cpu_em; (void)emul; (void)fault_address;

	insn = get_faulting_instruction(state);
	state->pc += 2;
	(void)insn;

	panic();
}

static void emulate_arm(const vm_cpu *vm_cpu_em, const emulate *emul, uintptr_t fault_address) {
	vm_cpu_state *state = current_vm_cpu->cpu_state;
	uint32_t insn;

	insn = get_faulting_instruction(state);
	// trace("em", state->cpsr, state->pc, state->hw_spsr, insn);

	state->pc += 4;

	if ((insn & 0x0f000010) == 0x0e000010) {
	/* MRC or MCR */
		uint32_t coproc_number = BITS(insn, 8, 4);
		uint32_t immediate0 = BITS(insn, 21, 3);
		uint32_t creg_n = BITS(insn, 16, 4);
		uint32_t creg_m = BITS(insn, 0, 4);
		uint32_t immediate1 = BITS(insn, 5, 3);
		uint32_t gpreg_number = BITS(insn, 12, 4);
		if (BITS(insn, 20, 1) == 0) {
			coproc_write(current_vm_cpu, coproc_number,
				(immediate0 << 12) | (creg_n << 8) | (creg_m << 4) | immediate1,
				state->gpregs[gpreg_number]);
		} else {
			state->gpregs[gpreg_number] = coproc_read(current_vm_cpu, coproc_number,
				(immediate0 << 12) | (creg_n << 8) | (creg_m << 4) | immediate1);
		}

		upcall();
	} else if ((insn & 0x0e100000) == 0x04100000) {
	/* LDR/LDRB rn, [rm, #off] (including pre- and post-increment variants) */
		// uint32_t bytesize = (insn & 0x00400000);
		uint32_t value_register_index = BITS(insn, 12, 4);
		// (only required for writeback)
		// uint32_t base_register_index = BITS(insn, 16, 4);
		// uint32_t immediate_offset = BITS(insn, 0, 12);
		uint32_t load_value = emulate_peripheral_load(vm_cpu_em, emul, fault_address);
		state->gpregs[value_register_index] = load_value;
	} else if ((insn & 0x0e100000) == 0x04000000) {
	/* STR/STRB rn, [rm, #off] (including pre- and post-increment variants) */
		// uint32_t bytesize = (insn & 0x00400000);
		uint32_t value_register_index = BITS(insn, 12, 4);
		// (only required for writeback)
		// uint32_t base_register_index = BITS(insn, 16, 4);
		// uint32_t immediate_offset = BITS(insn, 0, 12);
		emulate_peripheral_store(vm_cpu_em, emul, fault_address, state->gpregs[value_register_index]);
	} else if ((insn & 0x0e5000f0) == 0x005000b0) {
	/* LDRH rn, [rm, #off] */
		uint32_t value_register_index = BITS(insn, 12, 4);
		uint32_t load_value = emulate_peripheral_load(vm_cpu_em, emul, fault_address);
		state->gpregs[value_register_index] = load_value & 0xffff;
	} else if ((insn & 0x0e5000f0) == 0x004000b0) {
	/* STRH rn, [rm, #off] */
		uint32_t value_register_index = BITS(insn, 12, 4);
		emulate_peripheral_store(vm_cpu_em, emul, fault_address, state->gpregs[value_register_index] & 0xffff);
	} else if (insn == 0xe7f001f2) {
		printf("Linux BUG()\r\n");
		panic();
	} else {
		printf("G %x:%x #UND %x\r\n", state->cpsr, state->pc, insn);
		vm_cpu_inject_undefined(vm_cpu_em);
		upcall();
	}
}

void vm_cpu_emulate_faulted(const vm_cpu *vm_cpu_fault, const emulate *emul, uintptr_t fault_address) {
	vm_cpu_state *state = current_vm_cpu->cpu_state;

	if (state->hw_spsr & PSR_THUMB) {
		emulate_thumb(vm_cpu_fault, emul, fault_address);
	} else {
		emulate_arm(vm_cpu_fault, emul, fault_address);
	}
}

/**
 * This is the entry point for #UND traps. We have to rewind PC here because
 * a) the distance to rewind depends on the execution state (Thumb?)
 * b) the generic vm_cpu_emulate_instruction() will again increment PC past
 *    the emulated instruction (as that function is also used for the #ABT
 *    path)
 */
void vm_cpu_emulate_undefined(const vm_cpu *vm_cpu_und) {
	vm_cpu_state *state = current_vm_cpu->cpu_state;

	if (state->hw_spsr & PSR_THUMB) {
		state->pc -= 2;
		emulate_thumb(vm_cpu_und, NULL, 0);
	} else {
		state->pc -= 4;
		emulate_arm(vm_cpu_und, NULL, 0);
	}
}
