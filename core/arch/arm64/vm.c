#include <phidias.h>
#include <vm.h>
#include <memarea.h>
#include <schedule.h>
#include <misc_ops.h>
#include <arch/cpu_state.h>
#include <arch/asm/sysregs.h>
#include <arch/asm/cpu_state.h>

void aa64panic() {
	const vm_cpu *vmvm = current_vm_cpu;
	const memarea *mem;
	const memarea *memref;
	uint64_t j0, j1, j2, j3;

	if (vmvm == NULL) {
		printf("vmvm == NULL\n");
		panic();
	}

	mem = vmvm->memareas + 0;
	memref = mem->reference_area;

	asm volatile("mrs %0, ELR_EL1\n\tmrs %1, SPSR_EL1\n\tmrs %2, ESR_EL1\n\tmrs %3, FAR_EL1"
			: "=r" (j0), "=r" (j1), "=r" (j2), "=r" (j3));

	printf("Current EL1 problems: ELR %x SPSR %x ESR %x FAR %x\r\n",
		j0, j1, j2, j3);

	if ((j0 >= mem->vaddr) && (j0 < mem->vaddr + mem->size)) {
		uint32_t *code = (uint32_t *)(j0 - mem->vaddr + memref->vaddr);
		printf("Code at ELR: %x %x %x %x\r\n", code[0], code[1], code[2], code[3]);
	} else {
		printf("ELR out of range\r\n");
	}

	panic();
}

void vm_cpu_initialize(const vm_cpu *new_vm_cpu) {
	vm_cpu_state *cpu_state = new_vm_cpu->cpu_state;

	memset(cpu_state, 0, sizeof(vm_cpu_state));

	cpu_state->sctlr_el1 = SCTLR_EL1_FIXEDBITS;

	if (cpu_state->lightweight_flags & LIGHTWEIGHT_NOEL1) {
		cpu_state->pstate = 0x1c0;
	} else {
		cpu_state->pstate = 0x1c5;
	}
	cpu_state->pc = new_vm_cpu->vm->boot_address;

	cpu_state->gpregs[0] = 0x8000;
}

void vm_cpu_dump_state(const vm_cpu *vm_cpu_x) {
	vm_cpu_state *cpu_state = vm_cpu_x->cpu_state;
	uint64_t regs[4];
	uint32_t i;

	printf("PSTATE/PC: %x/%x\r\n", cpu_state->pstate, cpu_state->pc);
	for (i = 0; i < 8; i++) {
		printf("%s%16x %16x %16x %16x\r\n",
			(i == 0) ? "GPREGS:\t" : "\t",
			cpu_state->gpregs[4*i],   cpu_state->gpregs[4*i+1],
			cpu_state->gpregs[4*i+2], cpu_state->gpregs[4*i+3]);
	}

	asm volatile("mrs %0, ELR_EL1\n\tmrs %1, SPSR_EL1\n\tmrs %2, ESR_EL1\n\tmrs %3, FAR_EL1"
			: "=r" (regs[0]), "=r" (regs[1]), "=r" (regs[2]), "=r" (regs[3]));
	printf("EL1 State: %x %x %x %x\r\n", regs[0], regs[1], regs[2], regs[3]);

	asm volatile("mrs %0, SCTLR_EL1\n\tmrs %1, TCR_EL1\n\tmrs %2, TTBR0_EL1\n\tmrs %3, TTBR1_EL1"
			: "=r" (regs[0]), "=r" (regs[1]), "=r" (regs[2]), "=r" (regs[3]));
	printf("Current Paging Controls: %x %x %x %x\r\n",
			regs[0], regs[1], regs[2], regs[3]);

#if 0
	regs[0] = cpu_state->gpregs[31] - 0xffffff8008000000 + 0x40000000;
	for (i = 0; i < 8; i++) {
		uint64_t *stk = (uint64_t *)(regs[0] + i*32);
		printf("Stk... %16x %16x %16x %16x\r\n",
			stk[0], stk[1], stk[2], stk[3]);
	}
#endif
}

void vm_cpu_inject_fault(const vm_cpu *vm_cpu_inject, uintptr_t address, uint32_t flags) {
	(void)vm_cpu_inject; (void)address; (void)flags;
}

void vm_cpu_inject_interrupt(const vm_cpu *vm_cpu_inject, uint32_t interrupt_number) {
	(void)vm_cpu_inject; (void)interrupt_number;
}
