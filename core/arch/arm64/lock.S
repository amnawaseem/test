#include <asm/assembler.h>

.global	spinlock_lock
.global	spinlock_unlock
.global	spinlock_trylock

spinlock_lock:
	mov	w2, #1
1:
	ldaxr	w1, [x0]
	cbnz	w1, 1b
	stxr	w3, w2, [x0]
	cbnz	w3, 1b

	ret
ASM_FUNCTION_END spinlock_lock

spinlock_unlock:
	mov	w1, #0
	stlr	w1, [x0]
	ret
ASM_FUNCTION_END spinlock_unlock

spinlock_trylock:
	mov	w2, #1

	ldaxr	w1, [x0]
	cbnz	w1, 1f
	stxr	w3, w2, [x0]
	cbnz	w3, 1f

	mov	x0, #0
	ret
1:
	mov	x0, #1
	ret
ASM_FUNCTION_END spinlock_trylock
