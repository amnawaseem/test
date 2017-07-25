#include <asm/assembler.h>
#include <asm/offsets.h>
#include <arch/cpu_state.h>
#include <arch/asm/cpu_ops.h>
#include <arch/asm/cp15_regs.h>
#include <arch/vbar.h>
#include <asm/memarea.h>

.global	arm_vector_prefetch_abort
.global	arm_vector_data_abort

.pushsection	.data, "aw"

abort_frame:
	.long	0
	.long	0

.popsection

/**
 * Handle #PAB and #DAB entries.
 * In the unlucky case that the kernel itself caused the abort, we just
 * oops and die. Otherwise, it's a VM stepping on an emulated range or
 * experiencing a shadow fault. Both are handled by vm_cpu_fault().
 */
arm_vector_prefetch_abort:
	ldr	sp, =abort_frame
	sub	lr, lr, #4
	str	lr, [sp, #ENTRYFRAME_OFFSET_LR]
	mrs	lr, SPSR
	str	lr, [sp, #ENTRYFRAME_OFFSET_SPSR]
	tst	lr, #0x0f
	mrcne	IFAR(r3)
	bne	arm_abort_panic

	// entry from USR: save the guest gpregs

	cps	#PSR_MODE_SUPERVISOR
	push	{r0-r3}

	ldr	r3, =abort_frame
	vm_cpu_state_save 0
	mrc	IFAR(r0)
	mrc	IFSR(r2)
	mov	r1, #MEMAREA_FLAG_X | MEMAREA_FLAG_R
	b	vm_cpu_fault
	.ltorg
ASM_FUNCTION_END arm_vector_prefetch_abort

arm_vector_data_abort:
	ldr	sp, =abort_frame
	sub	lr, lr, #8
	str	lr, [sp, #ENTRYFRAME_OFFSET_LR]
	mrs	lr, SPSR
	str	lr, [sp, #ENTRYFRAME_OFFSET_SPSR]
	tst	lr, #0x0f
	mrcne	DFAR(r3)
	bne	arm_abort_panic

	// entry from USR: save the guest gpregs

	cps	#PSR_MODE_SUPERVISOR
	push	{r0-r3}

	ldr	r3, =abort_frame
	vm_cpu_state_save 0
	mrc	DFAR(r0)
	mrc	DFSR(r2)
	mov	r1, #MEMAREA_FLAG_R
	tst	r2, #FSR_BIT_WRITE
	orrne	r1, r1, #MEMAREA_FLAG_W
	b	vm_cpu_fault
	.ltorg
ASM_FUNCTION_END arm_vector_data_abort

arm_abort_panic:
	mov	r1, lr
	ldr	r2, [sp, #ENTRYFRAME_OFFSET_LR]
	cps	#PSR_MODE_SUPERVISOR
	ldr	r0, =arm_abort_panic_string
	bl	printf
	b	panic
	.ltorg
ASM_FUNCTION_END arm_abort_panic

.pushsection	.rodata, "a"

arm_abort_panic_string:
	.asciz	"Abort Panic: %x/%x (%x)\r\n"

.popsection
