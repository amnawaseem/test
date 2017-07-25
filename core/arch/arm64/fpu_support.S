#include <asm/assembler.h>

.global	divide_u64_u64
.global	modulo_u64_u64

divide_u64_u64:
	udiv	x2, x0, x1
	mov	x0, x2
	ret
ASM_FUNCTION_END divide_u64_u64

modulo_u64_u64:
	udiv	x2, x0, x1
	msub	x3, x2, x1, x0
	mov	x0, x3
	ret
ASM_FUNCTION_END modulo_u64_u64
