#ifndef _PHIDIAS_MIPS_TLB_H_
#define _PHIDIAS_MIPS_TLB_H_

struct tlb_entry_ {
	uint32_t	vaddr;
	uint32_t	paddr0;
	uint32_t	paddr1;
	uint32_t	size;
	uint8_t		flags;
};
typedef struct tlb_entry_ tlb_entry;

extern void write_tlb_entry(const memarea*, uint32_t bad_adress);

#endif /* _PHIDIAS_MIPS_TLB_H_ */
