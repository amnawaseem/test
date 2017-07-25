#include <phidias.h>
#include <vm.h>
#include <emulate/core.h>
#include <arch/cpu_state.h>

void vm_cpu_emulate_faulted(const vm_cpu *vm_cpu_fault, const emulate *emul, uintptr_t fault_address) {
	uint64_t reg_no;
	vm_cpu_state *cpustate = vm_cpu_fault->cpu_state;

	cpustate->pc += 4;

	if (cpustate->syndrome & 0x01000000) {
		// ISV is set, convenient emulation follows...
		reg_no = (cpustate->syndrome & 0x000f0000) >> 16;

		if (cpustate->syndrome & 0x40) {
			// it's a write
			emulate_peripheral_store(vm_cpu_fault, emul, fault_address, cpustate->gpregs[reg_no]);
		} else {
			// it's a read
			cpustate->gpregs[reg_no] = emulate_peripheral_load(vm_cpu_fault, emul, fault_address);
		}
	} else {
		printf("Fault w/o ISV, Addr %x\r\n", fault_address);
		aa64panic();
	}
}
