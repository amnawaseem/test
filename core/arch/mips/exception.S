#include <asm/assembler.h>
#include <asm/offsets.h>
#include <arch/cpu_state.h>

.globl	_timer_interrupt
.globl	_clock_interrupt

.pushsection	.exc_handler, "ax"
_general_exception_context_a:
	vm_cpu_state_save
	j	general_exception_handler
	nop
ASM_FUNCTION_END _general_exception_context_a
.popsection

.pushsection	.tlb_handler, "ax"
_tlb_exception_context:
	vm_cpu_state_save
	j	tlb_miss_handler
	nop
ASM_FUNCTION_END _tlb_exception_context
.popsection

.pushsection	.spc_handler, "ax"
_spc_context:
	vm_cpu_state_save
	j	special_interrupt_handler
	nop
ASM_FUNCTION_END _spc_context
.popsection

.pushsection	.isr, "ax"

_timer_interrupt:
	vm_cpu_state_save
	j	timer_pic32_interrupt
	nop
ASM_FUNCTION_END _timer_interrupt

_clock_interrupt:
	vm_cpu_state_save
	j	clock_pic32_interrupt
	nop
ASM_FUNCTION_END _clock_interrupt
.popsection
