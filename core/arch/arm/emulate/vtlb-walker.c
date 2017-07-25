#include <phidias.h>
#include <vm.h>
#include <misc_ops.h>
#include <emulate/core.h>
#include <emulate/vtlb.h>
#include <arch/cpu_state.h>
#include <arch/cpu_ops.h>
#include <arch/cp15_regs.h>

uintptr_t emulate_vtlb_walk_armv7long(const vm_cpu *vm_cpu_fault, uintptr_t address,
				uintptr_t *base, uintptr_t *size, uint32_t *flags) {
	(void)vm_cpu_fault; (void)address; (void)base; (void)size; (void)flags;

	printf("WALK LONG\r\n");
	panic();
}

uintptr_t emulate_vtlb_walk_armv7short(const vm_cpu *vm_cpu_fault, uintptr_t address,
				uintptr_t *base, uintptr_t *size, uint32_t *flags) {
	const memarea *translation_area;
	emulate_vtlb *vtlb = vm_cpu_fault->vtlb_emulate->control.vtlb;
	uintptr_t translation_offset;
	uintptr_t entry_location;
	uint32_t entry;
	uint32_t index;

	entry_location = vtlb->instances[vtlb->active_instance].vm_pagetable_address & 0xffffc000;

	translation_area = find_memarea_by_va(vm_cpu_fault->memarea_tree, entry_location);
	if (translation_area == NULL) {
		return 0;
	}
	translation_offset = translation_area->reference_area->vaddr - translation_area->vaddr;

	entry_location += translation_offset;
	index = (address >> 20);
	entry_location += (index << 2);
	entry = *(uint32_t *)entry_location;

	switch (entry & 3) {
	case 2:
		*flags = MEMAREA_FLAG_R;
		if (!(entry & 0x8000))		*flags |= MEMAREA_FLAG_W;
		if (!(entry & 0x10))		*flags |= MEMAREA_FLAG_X;
		if (!(entry & 0x20000))		*flags |= MEMAREA_FLAG_G;
		if ( (entry & 0x4))		*flags |= MEMAREA_FLAG_D;
		if ( (entry & 0x800))		*flags |= MEMAREA_FLAG_U;
		if ( (entry & 0x10000))		*flags |= MEMAREA_FLAG_S;
		*size = (1U << 20);
		*base = (entry & 0xfff00000);
		return *base + (address & 0x000fffff);
	case 1:
		break;
	default:
		return 0;
	} 

	entry_location = entry & 0xfffffc00;

	translation_area = find_memarea_by_va(vm_cpu_fault->memarea_tree, entry_location);
	if (translation_area == NULL) {
		return 0;
	}
	translation_offset = translation_area->reference_area->vaddr - translation_area->vaddr;

	entry_location += translation_offset;
	index = (address >> 12) & 0xff;
	entry_location += (index << 2);
	entry = *(uint32_t *)entry_location;

	if ((entry & 2) != 2) {
		return 0;
	}

	*flags = MEMAREA_FLAG_R;
	if (!(entry & 0x200))		*flags |= MEMAREA_FLAG_W;
	if (!(entry & 0x1))		*flags |= MEMAREA_FLAG_X;
	if (!(entry & 0x800))		*flags |= MEMAREA_FLAG_G;
	if ( (entry & 0x4))		*flags |= MEMAREA_FLAG_D;
	if ( (entry & 0x20))		*flags |= MEMAREA_FLAG_U;
	if ( (entry & 0x400))		*flags |= MEMAREA_FLAG_S;
	*size = (1U << 12);
	*base = (entry & 0xfffff000);
	return *base + (address & 0x00000fff);
}

/* Walker */

uintptr_t emulate_vtlb_walk(const vm_cpu *vm_cpu_fault, uintptr_t address, uintptr_t *base, uintptr_t *size, uint32_t *flags) {
	uintptr_t walked_address;

	*size = 0;
	*flags = 0;

        // printf("G %x:%x #ABT %8x\r\n", vm_cpu_fault->cpu_state->cpsr, vm_cpu_fault->cpu_state->pc, address);

	if ((vm_cpu_fault->cpu_state->sctlr & SCTLR_BIT_M) == 0) {
		*base = address & ~((1UL << 20) - 1);
		*size = 1U << 20;
		*flags = MEMAREA_FLAG_R | MEMAREA_FLAG_W | MEMAREA_FLAG_X | MEMAREA_FLAG_G | MEMAREA_FLAG_U | MEMAREA_FLAG_S;
		return address;
	}

	emulate_vtlb_ensure_host_paging();

	if (vm_cpu_fault->cpu_state->ttbcr & TTBCR_BIT_EAE) {
		walked_address = emulate_vtlb_walk_armv7long(vm_cpu_fault, address, base, size, flags);
	} else {
		walked_address = emulate_vtlb_walk_armv7short(vm_cpu_fault, address, base, size, flags);
	}

	emulate_vtlb_ensure_guest_paging(vm_cpu_fault);

	return walked_address;
}
