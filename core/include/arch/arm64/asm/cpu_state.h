#ifndef	_PHIDIAS_ARCH_ARM64_ASM_CPU_STATE_H_
#define	_PHIDIAS_ARCH_ARM64_ASM_CPU_STATE_H_

# define	LIGHTWEIGHT_NOFPU		0x00000001
# define	LIGHTWEIGHT_NOEL1		0x00000010

# ifdef __ASSEMBLER__

#if offset(vm_cpu_state, pstate) + 8 != offset(vm_cpu_state, pc)
# error Structure layout assumption violated!
#endif

	.macro	vm_cpu_state_save
	stp	x0, x1, [sp, #-16]!
	ldr	x0, =current_vm_cpu
	ldr	x0, [x0]
	ldr	x1, [x0, #offset(vm_cpu, cpu_state)]
	stp	x2, x3, [x1, #offset(vm_cpu_state, gpregs) + 2*8]
	stp	x4, x5, [x1, #offset(vm_cpu_state, gpregs) + 4*8]
	stp	x6, x7, [x1, #offset(vm_cpu_state, gpregs) + 6*8]
	stp	x8, x9, [x1, #offset(vm_cpu_state, gpregs) + 8*8]
	stp	x10, x11, [x1, #offset(vm_cpu_state, gpregs) + 10*8]
	stp	x12, x13, [x1, #offset(vm_cpu_state, gpregs) + 12*8]
	stp	x14, x15, [x1, #offset(vm_cpu_state, gpregs) + 14*8]
	stp	x16, x17, [x1, #offset(vm_cpu_state, gpregs) + 16*8]
	stp	x18, x19, [x1, #offset(vm_cpu_state, gpregs) + 18*8]
	stp	x20, x21, [x1, #offset(vm_cpu_state, gpregs) + 20*8]
	stp	x22, x23, [x1, #offset(vm_cpu_state, gpregs) + 22*8]
	stp	x24, x25, [x1, #offset(vm_cpu_state, gpregs) + 24*8]
	stp	x26, x27, [x1, #offset(vm_cpu_state, gpregs) + 26*8]
	stp	x28, x29, [x1, #offset(vm_cpu_state, gpregs) + 28*8]
	str	x30, [x1, #offset(vm_cpu_state, gpregs) + 30*8]
	mrs	x2, SP_EL1
	str	x2, [x1, #offset(vm_cpu_state, gpregs) + 31*8]
	ldp	x2, x3, [sp], #16
	mrs	x4, SPSR_EL2
	mrs	x5, ELR_EL2
	mrs	x6, ESR_EL2
	stp	x2, x3, [x1, #offset(vm_cpu_state, gpregs)]
	stp	x4, x5, [x1, #offset(vm_cpu_state, pstate)]
	str	x6, [x1, #offset(vm_cpu_state, syndrome)]
	.endm

	// Load the GPREGS state

	.macro	vm_cpu_state_load
	ldr	x0, =current_vm_cpu
	ldr	x0, [x0]
	ldr	x1, [x0, #offset(vm_cpu, cpu_state)]
	ldp	x2, x3, [x1, #offset(vm_cpu_state, pstate)]
	msr	SPSR_EL2, x2
	msr	ELR_EL2, x3
	ldp	x2, x3, [x1, #offset(vm_cpu_state, gpregs) + 2*8]
	ldp	x4, x5, [x1, #offset(vm_cpu_state, gpregs) + 4*8]
	ldp	x6, x7, [x1, #offset(vm_cpu_state, gpregs) + 6*8]
	ldp	x8, x9, [x1, #offset(vm_cpu_state, gpregs) + 8*8]
	ldp	x10, x11, [x1, #offset(vm_cpu_state, gpregs) + 10*8]
	ldp	x12, x13, [x1, #offset(vm_cpu_state, gpregs) + 12*8]
	ldp	x14, x15, [x1, #offset(vm_cpu_state, gpregs) + 14*8]
	ldp	x16, x17, [x1, #offset(vm_cpu_state, gpregs) + 16*8]
	ldp	x18, x19, [x1, #offset(vm_cpu_state, gpregs) + 18*8]
	ldp	x20, x21, [x1, #offset(vm_cpu_state, gpregs) + 20*8]
	ldp	x22, x23, [x1, #offset(vm_cpu_state, gpregs) + 22*8]
	ldp	x24, x25, [x1, #offset(vm_cpu_state, gpregs) + 24*8]
	ldp	x26, x27, [x1, #offset(vm_cpu_state, gpregs) + 26*8]
	ldp	x28, x29, [x1, #offset(vm_cpu_state, gpregs) + 28*8]
	ldp	x30, x0, [x1, #offset(vm_cpu_state, gpregs) + 30*8]
	msr	SP_EL1, x0
	ldp	x0, x1, [x1, #offset(vm_cpu_state, gpregs)]
	.endm

# endif
#endif
