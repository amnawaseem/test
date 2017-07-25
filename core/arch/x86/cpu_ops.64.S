#include <asm/assembler.h>

.global	cpu_idle
.global	cpu_zombie
.global	tlb_flush
.global	cpuid
.global	rdmsr

cpu_idle:
#if 0
	lea	.Lidleprint(%rip), %rdi
	xor	%eax, %eax
	callq	printf
#endif
	mov	top_of_stack(%rip), %rsp
1:
#if 0
	mov	$0xfffffffff409c000, %rcx
	mov	0x390(%rcx), %esi
	lea	.Llow(%rip), %rdi
	xor	%eax, %eax
	callq	printf
	int3
	sti
#else
	sti
	hlt
#endif
	jmp	1b
.Lidleprint:
	.asciz	"Entering idle...\r\n"
.Llow:
	.asciz	"Timer getting low: %x\r\n"

cpu_zombie:
	hlt
	jmp	cpu_zombie

tlb_flush:
	mov	%cr3, %rax
	mov	%rax, %cr3
	ret

cpuid:
	mov	%rsi, %rax
	mov	%rdx, %rcx
	cpuid
	mov	%rax, (%rdi)
	mov	%rbx, 8(%rdi)
	mov	%rcx, 16(%rdi)
	mov	%rdx, 24(%rdi)
	ret

rdmsr:
	mov	%rdx, %rcx
	rdmsr
	mov	%rdx, (%rdi)
	mov	%rax, (%rsi)
	ret

.pushsection .data, "aw"

.global	top_of_stack

top_of_stack:
	.quad	0

.popsection
