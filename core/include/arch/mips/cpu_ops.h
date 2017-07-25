#ifndef	_PHIDIAS_ARCH_MIPS_CPU_OPS_H_
#define	_PHIDIAS_ARCH_MIPS_CPU_OPS_H_

# include <arch/asm/cpu_ops.h>
# include <arch/cp0_regs.h>

extern uint32_t top_of_stack;
extern volatile uint32_t _gp;

/* INT on, loop */
static inline void cpu_idle() {
	printf("idling\r\n");
	asm volatile("ei \r\n ehb");
	while(1);
}

/* INT off, loop */
static inline void cpu_zombie() {
	printf("dieing\r\n");
	asm volatile("di \r\n ehb");
	while(1);
}

static inline void tlb_flush(uintptr_t address, uint32_t identifier) {
	(void)address;
	(void)identifier;
}

extern void cpu_dcache_flush(int do_clean, int do_invalidate);
extern unsigned short getGuestID(void);
extern void setGuestID(unsigned int);
extern unsigned short getRootGuestID(void);
extern void setRootGuestID(unsigned int);
extern void wtpss(uint32_t val, uint32_t reg);
extern uint32_t rfpss(uint32_t reg);
extern uint32_t rfss(uint32_t reg, uint32_t set);
extern uint32_t wtss(uint32_t val, uint32_t reg, uint32_t set);

static inline void advance_epc(uint16_t amt){
#ifdef __NO_INLINE__
	asm volatile (" mfc0	$t0, $14	\n\
			addu	$t0, $t0, %0	\n\
			mtc0	$t0, $14" 	: : "r" (amt));
#else
	asm volatile (" mfc0	$t0, $14	\n\
			addiu	$t0, $t0, %0	\n\
			mtc0	$t0, $14" 	: : "i" (amt));
#endif
}
#endif
