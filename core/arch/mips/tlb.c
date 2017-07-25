#include <phidias.h>
#include <debug.h>
#include <arch/tlb.h>
#include <arch/cpu_ops.h>
#include <arch/asm/cp0_regs.h>
#include <memarea.h>
#include <arch/cpu_state.h>
#include <vm.h>
#include <schedule.h>

uint8_t _cur_tlb_index = 0;
uint8_t max_tlb_index = 0;

void write_tlb_entry(const memarea *mem, uint32_t _tlb_bad_addr){
	/* TODO: check validity of entryLo1 */

	uint32_t page_size = 0x1000;
	uint32_t page_size_bits = 0;
	uint32_t entry_lo0, entry_lo1;

	/* make page as large as possible */
	while (page_size <= mem->size/4){
		page_size <<= 2;
		page_size_bits <<= 2;
		page_size_bits |=  3;
	}

	/* check whether requested addr is in this section of the memarea */
	if (_tlb_bad_addr > (mem->vaddr + 2*page_size)){
		/* it's not, recurse call with modified memarea
		 * only containing everything after the big page */
		memarea mem_cpy = {	mem->paddr + page_size,
					mem->vaddr + page_size,
					mem->size  - page_size,
					mem->flags,
					0	/* refernce_area not needed */
				  };
		write_tlb_entry(&mem_cpy, _tlb_bad_addr);
		upcall();
	}

	page_size_bits <<= 13;

	entry_lo0 = (( mem->paddr & 0x1FFFFFFF) );
	entry_lo0 >>= 6;
	entry_lo0  |= mem->flags | 0x4;

	entry_lo1 = ((mem->paddr & 0x1FFFFFFF) );
	entry_lo1  += page_size;
	entry_lo1 >>= 6;
	entry_lo1  |= mem->flags | 0x4;

	mtc0(c0_index, _cur_tlb_index);
	mtc0(c0_pagemask, page_size_bits);
	mtc0(c0_entrylo0, entry_lo0);
	mtc0(c0_entrylo1, entry_lo1);
	asm volatile("ehb");
	tlb_commit();
	printf("Written VA %08x -> PA %08x (and VA %08x -> PA %08x) in Ind: %d, BadVaddr: %08x\r\n",
			mem->vaddr,		(mfc0(c0_entrylo0) & ~0x1f) << 6,
			mem->vaddr + page_size, (mfc0(c0_entrylo1) & ~0x1f) << 6,
			_cur_tlb_index, _tlb_bad_addr);
	_cur_tlb_index++;
	if (_cur_tlb_index == max_tlb_index + 1)
		_cur_tlb_index = 0;
	upcall();
}
