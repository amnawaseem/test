#include <phidias.h>
#include <vm.h>
#include <schedule.h>
#include <arch/vm.h>
#include <arch/cpu_state.h>
#include <arch/cpu_ops.h>
#include <trace.h>

void vm_cpu_hypercall() {
	vm_cpu_state *state = current_vm_cpu->cpu_state;
	uint32_t code = state->gpregs[0];
	uint32_t *ptr;
	uint32_t tmp;

	if ((state->cpsr & PSR_MODEMASK) == PSR_MODE_USER) {
		// printf("SVC 0x%x\r\n", state->gpregs[7]);
		vm_cpu_inject_supervisorcall(current_vm_cpu);
		upcall();
	}

	// trace("h", state->cpsr, state->pc, state->hw_spsr, state->gpregs[0]);

	if ((code & HYPERCALLMASK_TYPE) != HYPERCALLTYPE_ARCHITECTURE) {
		vm_cpu_generic_hypercall(code, state->gpregs[1], &state->gpregs[0]);
		upcall();
	}

	switch (code) {
	case HYPERCALL_CPSR_READ:
		state->gpregs[0] = psr_mix_mode_flags(state->cpsr, state->hw_spsr);
		break;
	case HYPERCALL_CPSR_WRITE:
		if ((state->gpregs[1] ^ state->cpsr) & PSR_MODEMASK) {
			vm_cpu_switch_bank(state, state->cpsr & PSR_MODEMASK,
					state->gpregs[1] & PSR_MODEMASK);
		}
		state->cpsr = state->gpregs[1];
		state->hw_spsr = psr_mix_mode_flags(state->hw_spsr, state->gpregs[1]);
		break;
	case HYPERCALL_SPSR_READ:
		state->gpregs[0] = state->gpregs[15];
		break;
	case HYPERCALL_SPSR_WRITE:
		state->gpregs[15] = state->gpregs[1];
		break;
	case HYPERCALL_CPSID_I:
		state->cpsr |= PSR_IRQMASK;
		break;
	case HYPERCALL_CPSIE_I:
		state->cpsr &= ~PSR_IRQMASK;
		break;
	case HYPERCALL_CPSID_F:
		state->cpsr |= PSR_FIQMASK;
		break;
	case HYPERCALL_CPSIE_F:
		state->cpsr &= ~PSR_FIQMASK;
		break;
	case HYPERCALL_SPR0_LRPC:
		state->gpregs[0] = state->gpregs[13];
		state->pc = state->gpregs[14];
		tmp = state->gpregs[15];
		if ((tmp ^ state->cpsr) & PSR_MODEMASK) {
			vm_cpu_switch_bank(state, state->cpsr & PSR_MODEMASK,
					tmp & PSR_MODEMASK);
		}
		state->cpsr = tmp;
		state->hw_spsr = psr_mix_mode_flags(state->hw_spsr, tmp);
		// trace("r2", state->cpsr, state->pc, 0, 0);
		break;
	case HYPERCALL_LR_EXCEPTION_RETURN(0) ... HYPERCALL_LR_EXCEPTION_RETURN(0xffff):
		ptr = (uint32_t *)state->gpregs[13];
		state->gpregs[0] = ptr[-1];
		state->pc = state->gpregs[14];
		tmp = state->gpregs[15];
		if ((tmp ^ state->cpsr) & PSR_MODEMASK) {
			vm_cpu_switch_bank(state, state->cpsr & PSR_MODEMASK,
					tmp & PSR_MODEMASK);
		}
		state->cpsr = tmp;
		state->hw_spsr = psr_mix_mode_flags(state->hw_spsr, tmp);
		// trace("r1", state->cpsr, state->pc, 0, 0);
		break;
	case HYPERCALL_LDM_EXCEPTION_RETURN(0x8000) ... HYPERCALL_LDM_EXCEPTION_RETURN(0xffff):
		ptr = (uint32_t *)state->gpregs[13];
		for (tmp = 0; tmp < 15; tmp++) {
			if (code & (1 << tmp)) {
				state->gpregs[tmp] = *ptr;
				ptr++;
			}
		}
		state->pc = *ptr;
		tmp = state->gpregs[15];
		if ((tmp ^ state->cpsr) & PSR_MODEMASK) {
			vm_cpu_switch_bank(state, state->cpsr & PSR_MODEMASK,
					tmp & PSR_MODEMASK);
		}
		state->cpsr = tmp;
		state->hw_spsr = psr_mix_mode_flags(state->hw_spsr, tmp);
		// trace("r+", state->cpsr, state->pc, 0, 0);
		break;
	case HYPERCALL_LDM_USER(0) ... HYPERCALL_LDM_USER(0x7fff):
		ptr = (uint32_t *)state->gpregs[1];
		for (tmp = 0; tmp < 15; tmp++) {
			if (code & (1 << tmp)) {
				if (tmp == 13) {
					state->regs_usr[0] = *ptr;
				} else if (tmp == 14) {
					state->regs_usr[1] = *ptr;
				} else {
					state->gpregs[tmp] = *ptr;
				}
				ptr++;
			}
		}
		break;
	case HYPERCALL_STM_USER(0) ... HYPERCALL_STM_USER(0x7fff):
		ptr = (uint32_t *)state->gpregs[1];
		for (tmp = 0; tmp < 15; tmp++) {
			if (code & (1 << tmp)) {
				if (tmp == 13) {
					*ptr = state->regs_usr[0];
				} else if (tmp == 14) {
					*ptr = state->regs_usr[1];
				} else {
					*ptr = state->gpregs[tmp];
				}
				ptr++;
			}
		}
		break;
	default:
		printf("G %x:%x HYP %8x\r\n", state->cpsr, state->pc, code);
		printf("Unknown ARM Hypercall %8x\r\n", state->gpregs[0]);
		vm_cpu_dump_state(current_vm_cpu);
		panic();
	}

	upcall();
}
