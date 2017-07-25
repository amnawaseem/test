#include <asm/assembler.h>
#include <arch/vbar.h>
#include <arch/asm/cpu_ops.h>
#include <arch/cpu_state.h>

.global	arm_vector_undefined

.pushsection	.data, "aw"

undef_frame:
	.long	0
	.long	0

.popsection

/**
 * Handle an #UND entry.
 * In the unlucky case that the kernel itself caused the exception, we just
 * oops and die. Otherwise, it's a paravirtualized VM trying to perform a
 * privileged instruction. vm_cpu_emulate_instruction() will take care of it.
 * Note that we do not subtract from entry LR here - we are never going to
 * retry the instruction that caused the exception.
 */
arm_vector_undefined:
	ldr	sp, =undef_frame
	str	lr, [sp, #ENTRYFRAME_OFFSET_LR]
	mrs	lr, SPSR
	str	lr, [sp, #ENTRYFRAME_OFFSET_SPSR]
	tst	lr, #0x0f
	bne	.Lpanic

	// entry from USR: save the guest gpregs

	cps	#PSR_MODE_SUPERVISOR
	push	{r0-r3}

	ldr	r3, =undef_frame
	vm_cpu_state_save 0
	ldr	r0, =current_vm_cpu
	ldr	r0, [r0]
	b	vm_cpu_emulate_undefined

.Lpanic:
	mov	r1, lr
	ldr	r2, [sp, #ENTRYFRAME_OFFSET_LR]
	cps	#PSR_MODE_SUPERVISOR
	ldr	r0, =arm_undefined_panic_string
	bl	printf
	b	panic
	.ltorg
ASM_FUNCTION_END arm_vector_undefined

.pushsection	.rodata, "a"

arm_undefined_panic_string:
	.asciz	"Undefined Panic: %x/%x\r\n"

.popsection
