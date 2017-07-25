#include <phidias.h>
#include <vm.h>
#include <misc_ops.h>
#include <arch/cpu_state.h>
#include <emulate/core.h>
#include <emulate/vtlb.h>

int emulate_vtlb_is_directory_entry(struct emulate_vtlb_ *vtlb, uint32_t level, uintptr_t entry_address) {
	uint32_t entry_value;

	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	entry_value = *(uint32_t *)entry_address;

	if ((level == 0) && ((entry_value & 0x81) == 0x01)) {
		return 1;
	}

	return 0;
}

uintptr_t emulate_vtlb_enter_directory(struct emulate_vtlb_ *vtlb, uint32_t level, uintptr_t entry_address) {
	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	return (*(uint32_t *)entry_address) & 0xfffff000;
}

void emulate_vtlb_make_directory_entry(struct emulate_vtlb_ *vtlb, uint32_t level,
                uintptr_t entry_address, uintptr_t directory_address) {
	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	*(uint32_t *)entry_address = directory_address | 0x007;
}

void emulate_vtlb_make_mapping_entry(struct emulate_vtlb_ *vtlb, uint32_t level,
                uintptr_t entry_address, uintptr_t address, uint32_t flags) {
	uint32_t hw_flags = 0x1;

	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	if ((flags & MEMAREA_FLAG_W))	hw_flags |= 0x2;
	if ((flags & MEMAREA_FLAG_X))	{;}
	if ((flags & MEMAREA_FLAG_G))	hw_flags |= 0x100;
	if ((flags & MEMAREA_FLAG_D))	hw_flags |= 0x1c;
	if ((flags & MEMAREA_FLAG_U))	hw_flags |= 0x4;
	if ((flags & MEMAREA_FLAG_S))	{;}

	if (level == 0)			hw_flags |= 0x80;

	printf("+MAP_E [%x] = %x\r\n", entry_address, address|hw_flags);
	*(uint32_t *)entry_address = address | hw_flags;
}
