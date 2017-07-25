#include <asm/assembler.h>

.global	spinlock_lock
.global	spinlock_unlock
.global	spinlock_trylock

spinlock_lock:
1:
	mov	(%rdi), %ecx
	test	%ecx, %ecx
	jnz	1b

	mov	$1, %ebx
	cmpxchg	%ebx, (%rdi)
	jnz	1b

	ret

spinlock_unlock:
	xor	%esi, %esi
	mov	%esi, (%rdi)

	ret

spinlock_trylock:
	ret
