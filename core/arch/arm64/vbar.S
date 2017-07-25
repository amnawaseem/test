#include <asm/assembler.h>
#include <asm/offsets.h>
#include <arch/asm/cpu_state.h>
#include <config.h>

nonimpl_vec_str:
	.asciz	"Nonimplemented Vec (off %qx) (from %qx:%qx) (synd %qx) (addr %qx)\r\n"

	.macro	nonimplemented_vector
1:
#ifdef FEATURE_DEBUG
	ldr	x0, =nonimpl_vec_str
	mov	x1, #1b - phidias_vbar
	mrs	x2, SPSR_EL2
	mrs	x3, ELR_EL2
	mrs	x4, ESR_EL2
	mrs	x5, FAR_EL2
	bl	printf
#endif
	b	aa64panic
	.ltorg
	.endm

.global	phidias_vbar

.align	12
phidias_vbar:

.align	7
vbar_sync_cel_sp0:
	nonimplemented_vector
.align	7
vbar_irq_cel_sp0:
	nonimplemented_vector
.align	7
vbar_fiq_cel_sp0:
	nonimplemented_vector
.align	7
vbar_serr_cel_sp0:
	nonimplemented_vector

.align	7
vbar_sync_cel_spx:
	nonimplemented_vector
.align	7
vbar_irq_cel_spx:
	b	interrupt_handler
.align	7
vbar_fiq_cel_spx:
	nonimplemented_vector
.align	7
vbar_serr_cel_spx:
	nonimplemented_vector

.align	7
vbar_sync_lel64:
	vm_cpu_state_save
	b	vm_arm64_trap
.align	7
vbar_irq_lel64:
	vm_cpu_state_save
	b	interrupt_handler
	.ltorg
.align	7
vbar_fiq_lel64:
	nonimplemented_vector
.align	7
vbar_serr_lel64:
	nonimplemented_vector

.align	7
vbar_sync_lel32:
	nonimplemented_vector
.align	7
vbar_irq_lel32:
	nonimplemented_vector
.align	7
vbar_fiq_lel32:
	nonimplemented_vector
.align	7
vbar_serr_lel32:
	nonimplemented_vector
