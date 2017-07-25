#include <asm/assembler.h>

.global	divide_u64_u64
.global	modulo_u64_u64

divide_u64_u64:
	xor	%rdx, %rdx
	mov	%rdi, %rax
	div	%rsi
	// quotient is already in %rax
	ret

modulo_u64_u64:
	xor	%rdx, %rdx
	mov	%rdi, %rax
	div	%rsi
	mov	%rdx, %rax
	ret
