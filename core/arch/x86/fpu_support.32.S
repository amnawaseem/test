#include <asm/assembler.h>

.global	divide_u64_u64
.global	modulo_u64_u64

// divide EDX:EAX by (sp+8):(sp+4), return EDX:EAX
divide_u64_u64:
	push	%ebx

	mov	0xc(%esp), %ebx
	test	%ebx, %ebx
	jz	1f

	int3
1:
	mov	0x8(%esp), %ebx
	div	%ebx

	mov	$0, %edx

	pop	%ebx
	ret

// compute remainder of division of
// EDX:EAX by (sp+8):(sp+4), return EDX:EAX
modulo_u64_u64:
	push	%ebx

	mov	0xc(%esp), %ebx
	test	%ebx, %ebx
	jz	1f

	int3
1:
	mov	0x8(%esp), %ebx
	div	%ebx

	mov	%edx, %eax
	mov	$0, %edx

	pop	%ebx
	ret
