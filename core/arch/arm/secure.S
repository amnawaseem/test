#include <asm/assembler.h>

.arch_extension	sec

.global	smc_call

smc_call:
	smc	#0
	bx	lr
ASM_FUNCTION_END smc_call
