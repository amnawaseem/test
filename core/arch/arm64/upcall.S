#include <asm/assembler.h>
#include <asm/offsets.h>
#include <arch/asm/cpu_state.h>

.global	vm_cpu_upcall

vm_cpu_upcall:
#if 0
	ldr	x0, =current_vm_cpu
	ldr	x0, [x0]
	ldr	x8, [x0, #offset(vm_cpu, cpu_state)]
	ldp	x1, x2, [x8, #offset(vm_cpu_state, pstate)]
	adr	x0, .Ldebug
	bl	printf
#endif
	ldr	x1, =top_of_stack
	ldr	x2, [x1]
	mov	sp, x2
	vm_cpu_state_load
	eret
.Ldebug:
	.asciz	"Upcall to %x:%x\r\n"
ASM_FUNCTION_END vm_cpu_upcall
