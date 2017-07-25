#include <phidias.h>
#include <vm.h>
#include <misc_ops.h>
#include <arch/cpu_state.h>
#include <emulate/core.h>
#include <emulate/vtlb.h>

uintptr_t walker_x86_long(const struct vm_cpu_ *vm_cpu_walk, uintptr_t ptbl_base,
		uintptr_t address,
		uintptr_t *base, uintptr_t *size, uint32_t *flags) {
	uintptr_t entry_location;
	uint64_t entry;
	const memarea *translation_area;
	uintptr_t translation_offset;
	uint32_t index[4];

	index[0] = (address >> 39) & 0x1ff;
	index[1] = (address >> 30) & 0x1ff;
	index[2] = (address >> 21) & 0x1ff;
	index[3] = (address >> 12) & 0x1ff;

	entry_location = ptbl_base;
	translation_area = find_memarea_by_va(vm_cpu_walk->memarea_tree, entry_location);
	if (translation_area == NULL) {
		return 0;
	}
	translation_offset = translation_area->reference_area->vaddr - translation_area->vaddr;
	entry_location += translation_offset + (index[0] << 3);
	entry = *(uint64_t *)entry_location;

	if (!(entry & 1)) {
		return 0;
	}

	entry_location = entry & 0x0000fffffffff000L;
	translation_area = find_memarea_by_va(vm_cpu_walk->memarea_tree, entry_location);
	if (translation_area == NULL) {
		return 0;
	}
	translation_offset = translation_area->reference_area->vaddr - translation_area->vaddr;
	entry_location += translation_offset + (index[1] << 3);
	entry = *(uint64_t *)entry_location;

	if ((entry & 0x81) == 0x81) {
		*flags = MEMAREA_FLAG_R;
		if (entry & 0x002)	*flags |= MEMAREA_FLAG_W;
		if (!(entry & 0x8000000000000000))	*flags |= MEMAREA_FLAG_X;
		if (entry & 0x004)	*flags |= MEMAREA_FLAG_U;
		if (entry & 0x018)	*flags |= MEMAREA_FLAG_D;
		if (entry & 0x100)	*flags |= MEMAREA_FLAG_G;
		*size = (1U << 30);
		*base = (entry & 0x0000ffffc0000000);
		return *base + (address & 0x3fffffff);
	}
	if (!(entry & 1)) {
		return 0;
	}

	entry_location = entry & 0x0000fffffffff000L;
	translation_area = find_memarea_by_va(vm_cpu_walk->memarea_tree, entry_location);
	if (translation_area == NULL) {
		return 0;
	}
	translation_offset = translation_area->reference_area->vaddr - translation_area->vaddr;
	entry_location += translation_offset + (index[2] << 3);
	entry = *(uint64_t *)entry_location;

	if ((entry & 0x81) == 0x81) {
		*flags = MEMAREA_FLAG_R;
		if (entry & 0x002)	*flags |= MEMAREA_FLAG_W;
		if (!(entry & 0x8000000000000000))	*flags |= MEMAREA_FLAG_X;
		if (entry & 0x004)	*flags |= MEMAREA_FLAG_U;
		if (entry & 0x018)	*flags |= MEMAREA_FLAG_D;
		if (entry & 0x100)	*flags |= MEMAREA_FLAG_G;
		*size = (1U << 21);
		*base = (entry & 0x0000ffffffe00000);
		return *base + (address & 0x001fffff);
	}
	if (!(entry & 1)) {
		return 0;
	}

	entry_location = entry & 0x0000fffffffff000L;
	translation_area = find_memarea_by_va(vm_cpu_walk->memarea_tree, entry_location);
	if (translation_area == NULL) {
		return 0;
	}
	translation_offset = translation_area->reference_area->vaddr - translation_area->vaddr;
	entry_location += translation_offset + (index[1] << 3);
	entry = *(uint64_t *)entry_location;

	if (!(entry & 1)) {
		return 0;
	}

	*flags = MEMAREA_FLAG_R;
	if (entry & 0x002)	*flags |= MEMAREA_FLAG_W;
	if (!(entry & 0x8000000000000000))	*flags |= MEMAREA_FLAG_X;
	if (entry & 0x004)	*flags |= MEMAREA_FLAG_U;
	if (entry & 0x018)	*flags |= MEMAREA_FLAG_D;
	if (entry & 0x100)	*flags |= MEMAREA_FLAG_G;
	*size = (1U << 12);
	*base = (entry & 0x0000fffffffff000);
	return *base + (address & 0xfff);
}

uintptr_t walker_x86_legacy(const struct vm_cpu_ *vm_cpu_walk, uintptr_t ptbl_base,
		uintptr_t address,
		uintptr_t *base, uintptr_t *size, uint32_t *flags) {
	uintptr_t entry_location;
	uintptr_t translation_offset;
	uint32_t entry;
	uint32_t index;
	const memarea *translation_area;

	entry_location = ptbl_base;

	translation_area = find_memarea_by_va(vm_cpu_walk->memarea_tree, entry_location);
	if (translation_area == NULL) {
		return 0;
	}
	translation_offset = translation_area->reference_area->vaddr - translation_area->vaddr;

	entry_location += translation_offset;
	index = (address >> 22);
	entry_location += (index << 2);
	entry = *(uint32_t *)entry_location;

	if ((entry & 0x81) == 0x81) {
		*flags = MEMAREA_FLAG_R;
		if (entry & 0x002)	*flags |= MEMAREA_FLAG_W;
		if (entry & 0x004)	*flags |= MEMAREA_FLAG_U;
		if (entry & 0x018)	*flags |= MEMAREA_FLAG_D;
		if (entry & 0x100)	*flags |= MEMAREA_FLAG_G;
		*size = (1U << 22);
		*base = (entry & 0xffc00000);
		return *base + (address & 0x003fffff);
	} else if (!(entry & 1)) {
		return 0;
	}

	entry_location = entry & 0xfffff000;

	translation_area = find_memarea_by_va(vm_cpu_walk->memarea_tree, entry_location);
	if (translation_area == NULL) {
		return 0;
	}
	translation_offset = translation_area->reference_area->vaddr - translation_area->vaddr;

	entry_location += translation_offset;
	index = (address >> 12) & 0x3ff;
	entry_location += (index << 2);
	entry = *(uint32_t *)entry_location;

	if (!(entry & 1)) {
		return 0;
	}

	*flags = MEMAREA_FLAG_R;
	if (entry & 0x002)	*flags |= MEMAREA_FLAG_W;
	if (entry & 0x004)	*flags |= MEMAREA_FLAG_U;
	if (entry & 0x018)	*flags |= MEMAREA_FLAG_D;
	if (entry & 0x100)	*flags |= MEMAREA_FLAG_G;
	*size = (1U << 12);
	*base = (entry & 0xfffff000);
	return *base + (address & 0x00000fff);
}

uintptr_t emulate_vtlb_walk(const struct vm_cpu_ *vm_cpu_walk, uintptr_t address,
		uintptr_t *base, uintptr_t *size, uint32_t *flags) {
	emulate_vtlb *vtlb = vm_cpu_walk->vtlb_emulate->control.vtlb;

	if (!(vm_cpu_walk->cpu_state->guest_cr0 & 0x80000000)) {
		*base = address & 0xffc00000;
		*size = 1U << 22;
		*flags = MEMAREA_FLAG_R | MEMAREA_FLAG_W | MEMAREA_FLAG_X | MEMAREA_FLAG_G | MEMAREA_FLAG_U | MEMAREA_FLAG_S;
		return address;
	}

	if (vm_cpu_walk->cpu_state->guest_cr4 & 0x20) {
		// PAE
		return 0;
	} else {
		// should check EFER.LMA (64 bit?)
		return walker_x86_legacy(vm_cpu_walk, vtlb->instances[vtlb->active_instance].vm_pagetable_address & 0xfffff000,
					address, base, size, flags);
	}
}

//

void emulate_vtlb_ensure_host_paging(void) {}

void emulate_vtlb_ensure_guest_paging(const struct vm_cpu_ *vm_cpu_sw) {
	const emulate_vtlb *vtlb = vm_cpu_sw->vtlb_emulate->control.vtlb;

	uintptr_t pagetable_base = vtlb->instances[vtlb->active_instance].pagetable_address;

	vm_cpu_sw->cpu_state->vmcb->cr3 = pagetable_base;
	printf("xCR3: %x\r\n", pagetable_base);
}
