#include <asm/assembler.h>
#include <asm/offsets.h>
#include <arch/asm/cp0_regs.h>

.global	vm_cpu_upcall
.set noat

vm_cpu_upcall:
	/* go */
	eret
ASM_FUNCTION_END vm_cpu_upcall
