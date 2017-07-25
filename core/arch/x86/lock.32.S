#include <asm/assembler.h>

.global	spinlock_lock
.global	spinlock_unlock
.global	spinlock_trylock

spinlock_lock:
	mov	%eax, %edx

1:
	mov	(%edx), %ecx
	test	%ecx, %ecx
	jnz	1b

	mov	$1, %ebx
	cmpxchg	%ebx, (%edx)
	jnz	1b

	ret

spinlock_unlock:
	xor	%edx, %edx
	mov	%edx, (%eax)

	ret

spinlock_trylock:
	ret
