#include <phidias.h>
#include <vm.h>
#include <arch/vm.h>
#include <arch/cpu_state.h>
#include <arch/cpu_ops.h>
#include <arch/cp15_regs.h>
#include <trace.h>

void vm_cpu_initialize(const vm_cpu *new_vm_cpu) {
	new_vm_cpu->cpu_state->hw_spsr = 0x10;
	new_vm_cpu->cpu_state->cpsr = 0x1d3;
	new_vm_cpu->cpu_state->sctlr = 0xc50850;
	new_vm_cpu->cpu_state->pc = new_vm_cpu->vm->boot_address;

	new_vm_cpu->cpu_state->mpidr = 0; // 0x80000000;

	new_vm_cpu->cpu_state->gpregs[1] = ~0;
	new_vm_cpu->cpu_state->gpregs[2] = 0x60ff0000;

	new_vm_cpu->cpu_state->fpscr = 0;
	new_vm_cpu->cpu_state->fpexc = 0;
}

void vm_cpu_dump_state(const vm_cpu *vm_cpu_x) {
	vm_cpu_state *state = vm_cpu_x->cpu_state;

	printf("GPRegs\t%8x %8x %8x %8x\r\n",
		state->gpregs[0], state->gpregs[1], state->gpregs[2], state->gpregs[3]);
	printf("\t%8x %8x %8x %8x\r\n",
		state->gpregs[4], state->gpregs[5], state->gpregs[6], state->gpregs[7]);
	printf("\t%8x %8x %8x %8x\r\n",
		state->gpregs[8], state->gpregs[9], state->gpregs[10], state->gpregs[11]);
	printf("\t%8x %8x %8x %8x\r\n",
		state->gpregs[12], state->gpregs[13], state->gpregs[14], state->gpregs[15]);

	printf("USR:\t%8x %8x\r\n",
		state->regs_usr[0], state->regs_usr[1]);
	printf("FIQ:\t%8x %8x %8x %8x\r\n",
		state->regs_fiq[0], state->regs_fiq[1], state->regs_fiq[2], state->regs_fiq[3]);
	printf("\t%8x %8x %8x %8x\r\n",
		state->regs_fiq[4], state->regs_fiq[5], state->regs_fiq[6], state->regs_fiq[7]);
	printf("IRQ:\t%8x %8x %8x\r\n",
		state->regs_irq[0], state->regs_irq[1], state->regs_irq[2]);
	printf("SVC:\t%8x %8x %8x\r\n",
		state->regs_svc[0], state->regs_svc[1], state->regs_svc[2]);
	printf("ABT:\t%8x %8x %8x\r\n",
		state->regs_abt[0], state->regs_abt[1], state->regs_abt[2]);
	printf("UND:\t%8x %8x %8x\r\n",
		state->regs_und[0], state->regs_und[1], state->regs_und[2]);

	printf("PC/PSRs\t%8x %8x %8x\r\n",
		state->pc, state->cpsr, state->hw_spsr);
	printf("TTB*\t%8x %8x %8x\r\n",
		state->ttbr0, state->ttbr1, state->ttbcr);
}

// --------------------------------------------------------------------------

void vm_cpu_inject_supervisorcall(const vm_cpu *vm_cpu_inject) {
	vm_cpu_state *state = vm_cpu_inject->cpu_state;

	if ((state->cpsr & PSR_MODEMASK) != PSR_MODE_SUPERVISOR) {
		vm_cpu_switch_bank(state, state->cpsr & PSR_MODEMASK, PSR_MODE_SUPERVISOR);
		state->gpregs[14] = state->pc;
		state->gpregs[15] = psr_mix_mode_flags(state->cpsr, state->hw_spsr);
		// TODO: check ThumbVecs in SCTLR, set or clear PSR_THUMB as well (also in hw_spsr!)
		state->cpsr = (state->cpsr & ~PSR_MODEMASK) | PSR_MODE_SUPERVISOR | PSR_IRQMASK;
	}
	// printf("INJ %x:%x -> SVC\r\n", state->gpregs[15], state->gpregs[14]);

	vm_cpu_enter_vector(state, VECTOROFFSET_SVC);
}

void vm_cpu_inject_fault(const vm_cpu *vm_cpu_inject, uintptr_t address, uint32_t flags) {
	vm_cpu_state *state = vm_cpu_inject->cpu_state;
	int is_prefetch = (flags & MEMAREA_FLAG_X);

	if ((state->cpsr & PSR_MODEMASK) != PSR_MODE_ABORT) {
		vm_cpu_switch_bank(state, state->cpsr & PSR_MODEMASK, PSR_MODE_ABORT);
		state->gpregs[14] = state->pc + (is_prefetch ? 4 : 8);
		state->gpregs[15] = psr_mix_mode_flags(state->cpsr, state->hw_spsr);
		// TODO: check ThumbVecs in SCTLR, set or clear PSR_THUMB as well (also in hw_spsr!)
		state->cpsr = (state->cpsr & ~PSR_MODEMASK) | PSR_MODE_ABORT | PSR_IRQMASK;
	}
	// printf("INJ %x:%x -> ABT\r\n", state->gpregs[15], state->gpregs[14]);

	if (is_prefetch) {
		state->ifar = address;
		state->ifsr = 0x5;
	} else {
		state->dfar = address;
		state->dfsr = 0x5;
		if (flags & MEMAREA_FLAG_W)
			state->dfsr |= 0x800;
	}

	vm_cpu_enter_vector(state, (is_prefetch ? VECTOROFFSET_PAB : VECTOROFFSET_DAB));
}

void vm_cpu_inject_interrupt(const vm_cpu *vm_cpu_inject, uint32_t interrupt_number) {
	vm_cpu_state *state = vm_cpu_inject->cpu_state;

	(void)interrupt_number;

	if (state->cpsr & PSR_IRQMASK) {
		return;
	}

	if ((state->cpsr & PSR_MODEMASK) != PSR_MODE_INTERRUPT) {
		vm_cpu_switch_bank(state, state->cpsr & PSR_MODEMASK, PSR_MODE_INTERRUPT);
		state->gpregs[14] = state->pc + 4;
		state->gpregs[15] = psr_mix_mode_flags(state->cpsr, state->hw_spsr);
		// TODO: check ThumbVecs in SCTLR, set or clear PSR_THUMB as well (also in hw_spsr!)
		state->cpsr = (state->cpsr & ~PSR_MODEMASK) | PSR_MODE_INTERRUPT | PSR_IRQMASK;
	}
	// trace("j", state->gpregs[14], state->gpregs[15], state->cpsr, state->hw_spsr);
	// printf("INJ %x:%x -> IRQ\r\n", state->gpregs[15], state->gpregs[14]);

	vm_cpu_enter_vector(state, VECTOROFFSET_IRQ);
}

void vm_cpu_inject_undefined(const vm_cpu *vm_cpu_inject) {
	vm_cpu_state *state = vm_cpu_inject->cpu_state;

	if ((state->cpsr & PSR_MODEMASK) != PSR_MODE_UNDEFINED) {
		vm_cpu_switch_bank(state, state->cpsr & PSR_MODEMASK, PSR_MODE_UNDEFINED);
		state->gpregs[14] = state->pc;
		state->gpregs[15] = psr_mix_mode_flags(state->cpsr, state->hw_spsr);
		// TODO: check ThumbVecs in SCTLR, set or clear PSR_THUMB as well (also in hw_spsr!)
		state->cpsr = (state->cpsr & ~PSR_MODEMASK) | PSR_MODE_UNDEFINED | PSR_IRQMASK;
	}
	// printf("INJ %x:%x -> UND\r\n", state->gpregs[15], state->gpregs[14]);

	vm_cpu_enter_vector(state, VECTOROFFSET_UND);
}

#define	regcopy(cnt, dst, src)		{ for (i = 0; i < (cnt); i++) (dst)[i] = (src)[i]; }

void vm_cpu_switch_bank(vm_cpu_state *state, uint32_t old_mode, uint32_t new_mode) {
	uint32_t i;

	switch (old_mode) {
	case PSR_MODE_USER:
	case PSR_MODE_SYSTEM:
		regcopy(2, state->regs_usr, state->gpregs+13); break;
	case PSR_MODE_FASTIRQ:
		regcopy(8, state->regs_fiq, state->gpregs+8);
		regcopy(5, state->gpregs+8, state->regs_nonfiq); break;
	case PSR_MODE_INTERRUPT:
		regcopy(3, state->regs_irq, state->gpregs+13); break;
	case PSR_MODE_SUPERVISOR:
		regcopy(3, state->regs_svc, state->gpregs+13); break;
	case PSR_MODE_ABORT:
		regcopy(3, state->regs_abt, state->gpregs+13); break;
	case PSR_MODE_UNDEFINED:
		regcopy(3, state->regs_und, state->gpregs+13); break;
	default:
		panic();
	}

	switch (new_mode) {
	case PSR_MODE_USER:
	case PSR_MODE_SYSTEM:
		regcopy(2, state->gpregs+13, state->regs_usr); break;
	case PSR_MODE_FASTIRQ:
		regcopy(5, state->regs_nonfiq, state->gpregs+8);
		regcopy(8, state->gpregs+8, state->regs_fiq); break;
	case PSR_MODE_INTERRUPT:
		regcopy(3, state->gpregs+13, state->regs_irq); break;
	case PSR_MODE_SUPERVISOR:
		regcopy(3, state->gpregs+13, state->regs_svc); break;
	case PSR_MODE_ABORT:
		regcopy(3, state->gpregs+13, state->regs_abt); break;
	case PSR_MODE_UNDEFINED:
		regcopy(3, state->gpregs+13, state->regs_und); break;
	default:
		panic();
	}

	// printf("ModeSw %x -> %x (R8: %x)\r\n", old_mode, new_mode, state->gpregs[8]);
}

void vm_cpu_enter_vector(vm_cpu_state *state, uint32_t vector_offset) {
	if (state->sctlr & SCTLR_BIT_V) {
		state->pc = 0xffff0000 + vector_offset;
	} else {
		state->pc = state->vbar + vector_offset;
	}
}
