#include <asm/assembler.h>
#include <arch/vbar.h>
#include <arch/asm/cpu_ops.h>
#include <arch/cpu_state.h>

.global	arm_vector_interrupt

.pushsection	.data, "aw"

irq_frame:
	.long	0
	.long	0

.popsection

/**
 * Handle an #IRQ entry.
 * As our kernel is non-preemptible and uses a continuation style, we never
 * have to worry about clobbering SVC state: we will only use the SPSR:LR
 * location for upcalls back to a VM. If we came from SVC, the kernel was
 * idling in WFI anyway, so there is no state to save.
 */
arm_vector_interrupt:
	ldr	sp, =irq_frame
	sub	lr, lr, #4
	str	lr, [sp, #ENTRYFRAME_OFFSET_LR]
	mrs	lr, SPSR
	str	lr, [sp, #ENTRYFRAME_OFFSET_SPSR]
	tst	lr, #0x0f
	bne	1f

	// entry from USR: save the guest gpregs

	cps	#PSR_MODE_SUPERVISOR
	push	{r0-r3}

	ldr	r3, =irq_frame
	vm_cpu_state_save 0
	b	interrupt_handler

1:	// entry from SVC: nothing to save
	cps	#PSR_MODE_SUPERVISOR
	b	interrupt_handler
	.ltorg
ASM_FUNCTION_END arm_vector_interrupt
