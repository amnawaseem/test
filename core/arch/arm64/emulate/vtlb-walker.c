#include <phidias.h>
#include <vm.h>
#include <emulate/vtlb.h>

uintptr_t emulate_vtlb_walk(const vm_cpu *vm_cpu_fault, uintptr_t address, uintptr_t *base, uintptr_t *size, uint32_t *flags) {
	(void)vm_cpu_fault; (void)address; (void)base; (void)flags;

	*size = 0;

	return 0;
}
