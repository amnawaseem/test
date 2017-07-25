#include <asm/assembler.h>
#include <arch/cpu_state.h>

.global	arm_vector_supervisor_call

/**
 * Handle an #SVC entry.
 */
arm_vector_supervisor_call:
	push	{r0-r3}
	vm_cpu_state_save 1
	b	vm_cpu_hypercall
	.ltorg
ASM_FUNCTION_END arm_vector_supervisor_call
