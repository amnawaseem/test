#include <phidias.h>
#include <vm.h>
#include <xcore.h>
#include <memarea.h>
#include <schedule.h>
#include <arch/cpu_state.h>
#include <arch/asm/sysregs.h>
#include <trace.h>

void vm_handle_fault(uint64_t fault_flags, uint64_t hw_flags) {
	uint64_t fault_address;
	uint64_t fault_intermediate_address;

	(void)hw_flags;

	asm volatile("mrs %0, FAR_EL2" : "=r" (fault_address));
	asm volatile("mrs %0, HPFAR_EL2\n\tlsl %0, %0, #8" : "=r" (fault_intermediate_address));

	fault_intermediate_address |= (fault_address & 0xfff);

	//printf("Guest fault at %x (-> %x)\r\n", fault_address, fault_intermediate_address);
	// vm_cpu_dump_state(current_vm_cpu);

	vm_cpu_fault_nested(fault_intermediate_address, fault_flags);
}

void vm_handle_hypercall() {
	const vm_cpu *vm_trap = current_vm_cpu;
	vm_cpu_state *vm_cpu_trap = vm_trap->cpu_state;

	switch (vm_cpu_trap->gpregs[0]) {
	case 0xaaaa:
		trace("aa64hyp", (uintptr_t)vm_trap, vm_cpu_trap->gpregs[1],
				vm_cpu_trap->gpregs[2], vm_cpu_trap->gpregs[3]);
		return;
	case 0x9999:
		capability_invoke(vm_cpu_trap->gpregs[1]);
		return;
	default:
		; //printf("Unknown hypercall %x\r\n", vm_cpu_trap->gpregs[0]);
	}
}

void vm_arm64_trap() {
	const vm_cpu *vm_trap = current_vm_cpu;
	vm_cpu_state *vm_cpu_trap = vm_trap->cpu_state;
	uint64_t fault_flags;

	// printf("AA64 Trap Syn %x ELR %x\r\n", vm_cpu_trap->syndrome, vm_cpu_trap->pc);

	switch (vm_cpu_trap->syndrome >> 26) {
	case 0x20:		/* Prefetch Abort */
		printf("Nested Prefetch Abort\r\n");
		/* fault_flags = MEMAREA_FLAG_X | MEMAREA_FLAG_R;
		vm_handle_fault(fault_flags, vm_cpu_trap->syndrome & ESR_ELX_PABORT_FSCMASK); */
        
		vm_cpu_dump_state(current_vm_cpu);
		sched_state_modify(current_sched_entity, SCHEDULER_STATE_ZOMBIE);
	case 0x24:		/* Data Abort */
		fault_flags = MEMAREA_FLAG_R;
		if (vm_cpu_trap->syndrome & ESR_ELX_DABORT_WNR) {
			fault_flags |= MEMAREA_FLAG_W;
		}
		vm_handle_fault(fault_flags, vm_cpu_trap->syndrome & ESR_ELX_DABORT_FSCMASK);
		break;
	case 0x11:		/* AArch32 SVC (TGE) */
	case 0x12:		/* AArch32 HVC */
	case 0x15:		/* AArch64 SVC (TGE) */
	case 0x16:		/* AArch64 HVC */
		vm_handle_hypercall();
		break;
	case 0x13:		/* intercepted SMC, AArch32 */
	case 0x17:		/* intercepted SMC, AArch64 */
		printf("Intercepted SMC(%x:%x) - returning success\r\n", vm_cpu_trap->gpregs[0], vm_cpu_trap->gpregs[1]);
		vm_cpu_trap->gpregs[0] = 0L;
		vm_cpu_trap->pc += 4;
		break;
	case 0x01:		/* WFI/WFE */
		vm_cpu_trap->pc += 4;
		sched_state_modify(current_sched_entity, SCHEDULER_STATE_WAITING);
	case 0x00:		/* unsupported instruction */
		printf("EC00\r\n");
		vm_cpu_trap->gpregs[1] = 0L;
		vm_cpu_trap->pc += 4;
		break;
	default:
		printf("Unknown / unhandled EC, syndrome %x\r\n", vm_cpu_trap->syndrome);
		vm_cpu_dump_state(current_vm_cpu);
		sched_state_modify(current_sched_entity, SCHEDULER_STATE_ZOMBIE);
	}

	vm_cpu_upcall();
}
