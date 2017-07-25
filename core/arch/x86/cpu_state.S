#include <config.h>
#include <asm/assembler.h>
#include <asm/offsets.h>

.global	vm_cpu_state_save_full
.global	vm_cpu_state_load_full

#if defined(FEATURE_VIRT_TYPE_SVM)

vm_cpu_state_save_full:
	ret

vm_cpu_state_load_full:
	ret

#elif defined(FEATURE_VIRT_TYPE_VMX)

vm_cpu_state_save_full:
	mov	current_vm_cpu, %rax
	mov	offset(vm_cpu, cpu_state)(%rax), %rax
	vmclear	offset(vm_cpu_state, vmcb_pa)(%rax)
	ret

vm_cpu_state_load_full:
	mov	current_vm_cpu, %rax
	mov	offset(vm_cpu, cpu_state)(%rax), %rax
	vmptrld	offset(vm_cpu_state, vmcb_pa)(%rax)
	ret

#endif
