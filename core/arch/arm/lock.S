#include <asm/assembler.h>

.global	spinlock_lock
.global	spinlock_unlock
.global	spinlock_trylock

spinlock_lock:
	mov	r2, #1
1:
	ldrex	r1, [r0]
	tst	r1, r1
	bne	1b

	strex	r3, r2, [r0]
	tst	r3, r3
	bne	1b

	bx	lr
ASM_FUNCTION_END spinlock_lock

spinlock_unlock:
	mov	r1, #0
	str	r1, [r0]
	bx	lr
ASM_FUNCTION_END spinlock_unlock

spinlock_trylock:
	mov	r2, #1

	ldrex	r1, [r0]
	tst	r1, r1
	movne	r0, #1
	bxne	lr

	strex	r3, r2, [r0]
	tst	r3, r3
	movne	r0, #1
	moveq	r0, #0
	bx	lr
ASM_FUNCTION_END spinlock_trylock
