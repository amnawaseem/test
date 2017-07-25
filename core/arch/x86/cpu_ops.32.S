#include <asm/assembler.h>

.global	cpu_idle
.global	cpu_zombie
.global	tlb_flush

cpu_idle:
	mov	top_of_stack, %esp
	sti
	hlt
	ret

cpu_zombie:
	hlt
	jmp	cpu_zombie

tlb_flush:
	mov	%cr3, %eax
	mov	%eax, %cr3
	ret

.pushsection .data, "aw"

.global	top_of_stack

top_of_stack:
	.long	0

.popsection
