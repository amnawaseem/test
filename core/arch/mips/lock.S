#include <asm/assembler.h>

.global	spinlock_lock
.global	spinlock_unlock
.global	spinlock_trylock

spinlock_lock:
	addi	$t0,$zero,1
1:
	lw		$t1, 0($a0)
	bnez	$t1, 1b
	sw		$t0, 0($a0)
	jr		$ra
ASM_FUNCTION_END spinlock_lock

spinlock_unlock:
	sw		$zero, 0($a0)
	jr		$ra
ASM_FUNCTION_END spinlock_unlock

spinlock_trylock:
	lw		$t1, 0($a0)
	bnez	$t1, 1f
	addi	$t0, $zero, 1
	sw		$t0, 0($a0)
	addi	$v0, $zero, 0
	jr		$ra
1:
	addi	$v0, $zero, 1
	jr		$ra
ASM_FUNCTION_END spinlock_trylock
