#include <asm/assembler.h>
#include <asm/offsets.h>

.global	vm_cpu_upcall

vm_cpu_upcall:
	mov	top_of_stack(%rip), %rsp
	sti
	nop	# interrupt window

	clgi

	mov	current_vm_cpu(%rip), %rax
	mov	offset(vm_cpu, cpu_state)(%rax), %rax

	mov	offset(vm_cpu_state, rbx)(%rax), %rbx
	mov	offset(vm_cpu_state, rcx)(%rax), %rcx
	mov	offset(vm_cpu_state, rdx)(%rax), %rdx
	mov	offset(vm_cpu_state, rsi)(%rax), %rsi
	mov	offset(vm_cpu_state, rdi)(%rax), %rdi
	mov	offset(vm_cpu_state, rbp)(%rax), %rbp

	mov	offset(vm_cpu_state, vmcb_pa)(%rax), %rax
	vmload	%rax
	vmrun	%rax

	mov	current_vm_cpu(%rip), %rax
	mov	offset(vm_cpu, cpu_state)(%rax), %rax
	
	mov	%rbx, offset(vm_cpu_state, rbx)(%rax)
	mov	%rcx, offset(vm_cpu_state, rcx)(%rax)
	mov	%rdx, offset(vm_cpu_state, rdx)(%rax)
	mov	%rsi, offset(vm_cpu_state, rsi)(%rax)
	mov	%rdi, offset(vm_cpu_state, rdi)(%rax)
	mov	%rbp, offset(vm_cpu_state, rbp)(%rax)

	mov	offset(vm_cpu_state, vmcb_pa)(%rax), %rax
	vmsave	%rax

	stgi

	jmp	vm_cpu_vmexit
