#include <asm/assembler.h>
#include <asm/offsets.h>
#include <arch/asm/vmx_regs.h>

.global	vmread
.global	vmwrite
.global	vmx_make_active

.global	vm_cpu_upcall
.global	vmx_exit_asm

vm_cpu_upcall:
	mov	current_vm_cpu(%rip), %rax
	mov	offset(vm_cpu, cpu_state)(%rax), %rax

	mov	offset(vm_cpu_state, rbx)(%rax), %rbx
	mov	offset(vm_cpu_state, rcx)(%rax), %rcx
	mov	offset(vm_cpu_state, rdx)(%rax), %rdx
	mov	offset(vm_cpu_state, rsi)(%rax), %rsi
	mov	offset(vm_cpu_state, rdi)(%rax), %rdi
	mov	offset(vm_cpu_state, rbp)(%rax), %rbp
	mov	offset(vm_cpu_state, r8)(%rax), %r8
	mov	offset(vm_cpu_state, r9)(%rax), %r9
	mov	offset(vm_cpu_state, r10)(%rax), %r10
	mov	offset(vm_cpu_state, r11)(%rax), %r11
	mov	offset(vm_cpu_state, r12)(%rax), %r12
	mov	offset(vm_cpu_state, r13)(%rax), %r13
	mov	offset(vm_cpu_state, r14)(%rax), %r14
	mov	offset(vm_cpu_state, r15)(%rax), %r15
	mov	offset(vm_cpu_state, rax)(%rax), %rax

	vmresume
	vmlaunch

	mov	top_of_stack(%rip), %rsp
	jmp	vm_cpu_vmexit
.Lxxx:
	.asciz	"IDT: %x %x\r\n"

vmx_exit_asm:
	push	%rax
	mov	current_vm_cpu(%rip), %rax
	mov	offset(vm_cpu, cpu_state)(%rax), %rax

	mov	%rbx, offset(vm_cpu_state, rbx)(%rax)
	mov	%rcx, offset(vm_cpu_state, rcx)(%rax)
	mov	%rdx, offset(vm_cpu_state, rdx)(%rax)
	mov	%rsi, offset(vm_cpu_state, rsi)(%rax)
	mov	%rdi, offset(vm_cpu_state, rdi)(%rax)
	mov	%rbp, offset(vm_cpu_state, rbp)(%rax)
	mov	%r8, offset(vm_cpu_state, r8)(%rax)
	mov	%r9, offset(vm_cpu_state, r9)(%rax)
	mov	%r10, offset(vm_cpu_state, r10)(%rax)
	mov	%r11, offset(vm_cpu_state, r11)(%rax)
	mov	%r12, offset(vm_cpu_state, r12)(%rax)
	mov	%r13, offset(vm_cpu_state, r13)(%rax)
	mov	%r14, offset(vm_cpu_state, r14)(%rax)
	mov	%r15, offset(vm_cpu_state, r15)(%rax)
	pop	%rbx
	mov	%rbx, offset(vm_cpu_state, rax)(%rax)
#if 0
	lea	.Lxxx(%rip), %rdi
	push	%rdi
	push	%rdi
	sidt	(%rsp)
	pop	%rsi
	pop	%rdx
	xor	%eax, %eax
	callq	printf
	int3
#endif
	jmp	vm_cpu_vmexit

vmread:
	vmread	%rdi, %rax
	ret

vmwrite:
	vmwrite	%rsi, %rdi
	ret

vmx_make_active:
	vmptrld	offset(vm_cpu_state, vmcb_pa)(%rdi)
	ret
