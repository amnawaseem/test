#include <asm/assembler.h>
#include <asm/offsets.h>

.global	vm_cpu_upcall

vm_cpu_upcall:
	ldr	r0, =current_vm_cpu
	ldr	r0, [r0]
	ldr	r1, [r0, #offset(vm_cpu, cpu_state)]
	add	r2, r1, #offset(vm_cpu_state, gpregs) + 16
	ldmia	r2, {r4-lr}^
	ldr	r3, [r1, #offset(vm_cpu_state, hw_spsr)]
	ldr	lr, [r1, #offset(vm_cpu_state, pc)]
	ldr	r0, =top_of_stack
	ldr	sp, [r0]
	tst	r3, #0x10
	beq	panic
	msr	SPSR_sfxc, r3
	ldmdb	r2, {r0-r3}
	movs	pc, lr
ASM_FUNCTION_END vm_cpu_upcall
