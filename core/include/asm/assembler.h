#ifndef	_PHIDIAS_ASM_ASSEMBLER_H_
#define	_PHIDIAS_ASM_ASSEMBLER_H_

	.macro	ASM_FUNCTION symbol
	.type	\symbol, %function
	.endm

	.macro	ASM_FUNCTION_END symbol
	ASM_FUNCTION	\symbol
	.size	\symbol, . - \symbol
	.endm

#endif
