#include <asm/assembler.h>
#include <asm/offsets.h>

.global	vm_cpu_upcall

vm_cpu_upcall:
	mov	top_of_stack, %esp
	sti
	nop	# interrupt window

	clgi

	mov	current_vm_cpu, %eax
	mov	offset(vm_cpu, cpu_state)(%eax), %eax

	mov	offset(vm_cpu_state, rbx)(%eax), %ebx
	mov	offset(vm_cpu_state, rcx)(%eax), %ecx
	mov	offset(vm_cpu_state, rdx)(%eax), %edx
	mov	offset(vm_cpu_state, rsi)(%eax), %esi
	mov	offset(vm_cpu_state, rdi)(%eax), %edi
	mov	offset(vm_cpu_state, rbp)(%eax), %ebp

	mov	offset(vm_cpu_state, vmcb_pa)(%eax), %eax
	vmload	%eax
	vmrun	%eax

	mov	current_vm_cpu, %eax
	mov	offset(vm_cpu, cpu_state)(%eax), %eax
	
	mov	%ebx, offset(vm_cpu_state, rbx)(%eax)
	mov	%ecx, offset(vm_cpu_state, rcx)(%eax)
	mov	%edx, offset(vm_cpu_state, rdx)(%eax)
	mov	%esi, offset(vm_cpu_state, rsi)(%eax)
	mov	%edi, offset(vm_cpu_state, rdi)(%eax)
	mov	%ebp, offset(vm_cpu_state, rbp)(%eax)

	mov	offset(vm_cpu_state, vmcb_pa)(%eax), %eax
	vmsave	%eax

	stgi

	jmp	vm_cpu_vmexit
