#include <asm/assembler.h>

.global	modulo_u64_u64
.global	divide_u64_u64

/**
 * Compute the remainder of the division of a0 by 10
 * This strips modulo down to u32/10. But that's the only use-case.
 */
modulo_u64_u64:
	addi	$t0, $zero, 10
	div		$a0, $t0
	mfhi	$v0
	addi	$v1, $zero, 0
	jr		$ra
ASM_FUNCTION_END modulo_u64_u64

/**
 * Divide a0 by 10
 * This strips divide down to u32/10. But that's the only use-case.
 */
divide_u64_u64:
	addi	$t0, $zero, 10
	div		$a0, $t0
	mflo	$v0
	addi	$v1, $zero, 0
	jr		$ra
ASM_FUNCTION_END divide_u64_u64
