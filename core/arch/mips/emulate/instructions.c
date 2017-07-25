#include <phidias.h>
#include <vm.h>
#include <misc_ops.h>
#include <schedule.h>
#include <emulate/core.h>
#include <arch/cp0_regs.h>
#include <arch/cpu_state.h>
#include <arch/cpu_ops.h>
#include <arch/vm.h>
#include <arch/tlb.h>

static inline uint32_t get_faulting_instruction(void) {
	return mfc0(c0_badinstr);
}

static void emulate_mips(const vm_cpu *vm_cpu_em, const emulate *emul, uintptr_t fault_address) {
	uint32_t insn;
	uint32_t reg_encoded, value;

	vm_cpu_state *state = current_vm_cpu->cpu_state;

	insn = get_faulting_instruction();

	switch ((insn & 0xFC000000) >> 26){
		/* opcode */
		case OPC_SB:
			reg_encoded = (insn >> 16) & 0x1f;
			value = rfpss(reg_encoded);
			emulate_peripheral_store(vm_cpu_em, emul, fault_address, value & 0xFF );
		break;
		case OPC_SH:
			reg_encoded = (insn >> 16) & 0x1f;
			value = rfpss(reg_encoded);
			emulate_peripheral_store(vm_cpu_em, emul, fault_address, value & 0xFFFF );
		break;
		case OPC_SW:
			reg_encoded = (insn >> 16) & 0x1f;
			value = rfpss(reg_encoded);
			emulate_peripheral_store(vm_cpu_em, emul, fault_address, value);
		break;
		case OPC_LHU:
			reg_encoded = (insn >> 16) & 0x1f;
			value = emulate_peripheral_load(vm_cpu_em, emul, fault_address);
			wtpss(value & 0xFFFF, reg_encoded);
		break;
		case OPC_LW:
			reg_encoded = (insn >> 16) & 0x1f;
			value = emulate_peripheral_load(vm_cpu_em, emul, fault_address);
			wtpss(value, reg_encoded);
		break;
		default:
			printf("G %08x #UND %08x access to %08x\r\n", state->pc, insn, fault_address);
			vm_cpu_inject_undefined(vm_cpu_em);
		break;
	}
	state->pc += 4;
}

void vm_cpu_emulate_faulted(const vm_cpu *vm_cpu_fault, const emulate *emul, uintptr_t fault_address) {
	emulate_mips(vm_cpu_fault, emul, fault_address);
}

/**
 * This is the entry point for #UND traps. We have to rewind PC here because
 * a) the distance to rewind depends on the execution state (microMIPS?)
 * b) the generic vm_cpu_emulate_instruction() will again increment PC past
 *    the emulated instruction (as that function is also used for the #ABT
 *    path)
 */
void vm_cpu_emulate_undefined(const vm_cpu *vm_cpu_und) {
	vm_cpu_state *state = current_vm_cpu->cpu_state;

	state->pc -= 4;
	emulate_mips(vm_cpu_und, NULL, 0);
}
