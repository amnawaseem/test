#include <phidias.h>
#include <vm.h>
#include <emulate/vtlb.h>

int emulate_vtlb_is_directory_entry(emulate_vtlb *vtlb, uint32_t level, uintptr_t entry_address) {
	uint64_t entry_value;

	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	entry_value = *(uint64_t *)entry_address;

	if ((level < 3) && ((entry_value & 0x3) == 0x3)) {
		return 1;
	}

	return 0;
}

uintptr_t emulate_vtlb_enter_directory(emulate_vtlb *vtlb, uint32_t level, uintptr_t entry_address) {
	uint64_t entry_value;

	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	entry_value = *(uint64_t *)entry_address;

	return (entry_value & ~0xfffUL);
}

void emulate_vtlb_make_directory_entry(emulate_vtlb *vtlb, uint32_t level, uintptr_t entry_address, uintptr_t directory_address) {
	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	*(uint64_t *)entry_address = directory_address | 0x003;
}

void emulate_vtlb_make_mapping_entry(emulate_vtlb *vtlb, uint32_t level, uintptr_t entry_address, uintptr_t address, uint32_t flags) {
	uint64_t hwflags = 0;

	emulate_vtlb_assert_bounds(vtlb, level, entry_address);

	// assert that level != 0

	if (level < 3) {
		hwflags = 0x461;
	} else {
		hwflags = 0x463;
	}

	// flags have fixed bit positions across all levels on ARMv8

	if (!(flags & MEMAREA_FLAG_W)) hwflags |= 0x80;
	if (!(flags & MEMAREA_FLAG_X)) hwflags |= 0x40000000000000;
	//if (!(flags & MEMAREA_FLAG_G)) hwflags |=
	if ( (flags & MEMAREA_FLAG_D)) hwflags |= 0x4;
	//if (!(flags & MEMAREA_FLAG_U)) hwflags |=
	if ( (flags & MEMAREA_FLAG_S)) hwflags |= 0x200;

	*(uint64_t *)entry_address = address | hwflags;
}

//

void emulate_vtlb_ensure_host_paging() {}
void emulate_vtlb_ensure_guest_paging(const vm_cpu *xvcpu) {}
