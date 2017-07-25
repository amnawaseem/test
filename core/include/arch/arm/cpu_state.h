#ifndef	__ASSEMBLER__

struct vm_cpu_state_ {
	uint32_t	gpregs[16];	// r0-r14, spsr
	uint32_t	pc;
	uint32_t	cpsr;
	uint32_t	hw_spsr;
	uint32_t	regs_usr[2];	// r13-r14
	uint32_t	regs_fiq[8];	// r8-r14, spsr
	uint32_t	regs_nonfiq[5];	// r8-r12
	uint32_t	regs_irq[3];	// r13-r14, spsr
	uint32_t	regs_svc[3];	// r13-r14, spsr
	uint32_t	regs_abt[3];	// r13-r14, spsr
	uint32_t	regs_und[3];	// r13-r14, spsr

	uint32_t	contextidr;

	uint32_t	tpidrprw;
	uint32_t	tpidruro;
	uint32_t	tpidrurw;

	uint32_t	sctlr;
	uint32_t	dacr;
	uint32_t	mpidr;
	uint32_t	ttbcr;
	uint32_t	ttbr0;
	uint32_t	ttbr1;

	uint32_t	prrr;
	uint32_t	nmrr;

	uint32_t	vbar;

	uint32_t	dfsr;
	uint32_t	ifsr;
	uint32_t	dfar;
	uint32_t	ifar;

	uint32_t	fpscr;
	uint32_t	fpexc;
	uint32_t	fpregs[32];
};
typedef struct vm_cpu_state_	vm_cpu_state;

#else

# include <asm/offsets.h>
# include <arch/vbar.h>

	// Save all registers that are in immediate danger of being
	// clobbered.
	// Expects:
	// - {r0-r3} have been pushed to the stack
	// - (direct == 0) r3 contains the address of the entry frame carrying SPSR:LR
	// - (direct == 1) SPSR:LR values are in their respective registers
	.macro	vm_cpu_state_save direct
	ldr	r0, =current_vm_cpu
	ldr	r0, [r0]
	ldr	r1, [r0, #offset(vm_cpu, cpu_state)]
	add	r2, r1, #offset(vm_cpu_state, gpregs) + 16
	stmia	r2, {r4-lr}^
	pop	{r4-r7}
	stmdb	r2, {r4-r7}
	.if	\direct == 0
	ldr	r8, [r3, #ENTRYFRAME_OFFSET_LR]
	ldr	r9, [r3, #ENTRYFRAME_OFFSET_SPSR]
	str	r8, [r1, #offset(vm_cpu_state, pc)]
	str	r9, [r1, #offset(vm_cpu_state, hw_spsr)]
	.else
	mrs	r9, SPSR
	str	lr, [r1, #offset(vm_cpu_state, pc)]
	str	r9, [r1, #offset(vm_cpu_state, hw_spsr)]
	.endif
	ldr	r7, [r1, #offset(vm_cpu_state, cpsr)]
	movw	r6, #0x1ff
	and	r7, r7, r6
	bic	r9, r9, r6
	orr	r7, r7, r9
	str	r7, [r1, #offset(vm_cpu_state, cpsr)]
	.endm

#endif
