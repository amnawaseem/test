#include <phidias.h>
#include <vm.h>
#include <emulate/core.h>
#include <emulate/vtlb.h>

int emulate_vtlb_is_directory_entry(emulate_vtlb *vtlb, uint32_t level, uintptr_t entry_address) {
	uint32_t entry_value;

	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	entry_value = *(uint32_t *)entry_address;

	if ((level == 0) && ((entry_value & 0x00000003) == 0x00000001)) {
		return 1;
	}

	return 0;
}

uintptr_t emulate_vtlb_enter_directory(emulate_vtlb *vtlb, uint32_t level, uintptr_t entry_address) {
	uint32_t entry_value;

	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	entry_value = *(uint32_t *)entry_address;

	return (entry_value & 0xfffffc00);
}

void emulate_vtlb_make_directory_entry(emulate_vtlb *vtlb, uint32_t level, uintptr_t entry_address, uintptr_t directory_address) {
	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	*(uint32_t *)entry_address = directory_address | 0x001;
}

void emulate_vtlb_make_mapping_entry(emulate_vtlb *vtlb, uint32_t level, uintptr_t entry_address, uintptr_t address, uint32_t flags) {
	uint32_t hw_flags = 0;

	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	if (level == 0) {
		hw_flags = 0x402;
		if (!(flags & MEMAREA_FLAG_W)) hw_flags |= 0x8000;
		if (!(flags & MEMAREA_FLAG_X)) hw_flags |= 0x10;
		if (!(flags & MEMAREA_FLAG_G)) hw_flags |= 0x20000;
		if ((flags & MEMAREA_FLAG_D)) hw_flags |= 0x4;
		if ((flags & MEMAREA_FLAG_U)) hw_flags |= 0x800;
		if ((flags & MEMAREA_FLAG_S)) hw_flags |= 0x10000;
	} else if (level == 1) {
		hw_flags = 0x12;
		if (!(flags & MEMAREA_FLAG_W)) hw_flags |= 0x200;
		if (!(flags & MEMAREA_FLAG_X)) hw_flags |= 0x1;
		if (!(flags & MEMAREA_FLAG_G)) hw_flags |= 0x800;
		if ((flags & MEMAREA_FLAG_D)) hw_flags |= 0x4;
		if ((flags & MEMAREA_FLAG_U)) hw_flags |= 0x20;
		if ((flags & MEMAREA_FLAG_S)) hw_flags |= 0x400;
	}

	*(uint32_t *)entry_address = address | hw_flags;
}
