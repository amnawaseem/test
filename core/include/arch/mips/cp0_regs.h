#ifndef _PHIDIAS_ARCH_ARM_CP15_REGS_H_
#define	_PHIDIAS_ARCH_ARM_CP15_REGS_H_

# include <arch/asm/cp0_regs.h>

/* Read from CP0 */
#define mfc0(cpreg)		_mfc0(cpreg)
#define _mfc0(reg, sel) ({ int32_t __value;			\
		asm volatile (								\
		"mfc0   %0, $%1, %2"						\
		: "=r" (__value) : "K" (reg), "K" (sel));	\
		__value; })

/* Write to CP0 */
#define mtc0(cpreg, value)	_mtc0(cpreg, value)
#define _mtc0(reg, sel, value) asm volatile (				\
		"mtc0   %0, $%1, %2"								\
		: : "r" ((uint32_t) (value)), "K" (reg), "K" (sel))

/* Read from Guest CP0 */
#define mfgc0(cpreg)		_mfgc0(cpreg)
#define _mfgc0(reg, sel) ({ int32_t __value;			\
		asm volatile (								\
		"mfgc0   %0, $%1, %2"						\
		: "=r" (__value) : "K" (reg), "K" (sel));	\
		__value; })

/* Write to Guest CP0 */
#define mtgc0(cpreg, value)	_mtgc0(cpreg, value)
#define _mtgc0(reg, sel, value) asm volatile (				\
		"mtgc0   %0, $%1, %2"                 				\
		: : "r" ((uint32_t) (value)), "K" (reg), "K" (sel))

/* write to previous gpr shadow */
#define MoveToPreviousGuestGPR(reg, value) asm volatile (	\
		"wrpgpr   $%0, %1"									\
		: : "K" (reg), "r" ((uint32_t) (value)))

/* read from previous gpr shadow */
#define MoveFromPreviousGuestGPR(reg) ({ int32_t __value;	\
		asm volatile (										\
		"rdpgpr   %0, $%1"									\
		: "=r" (__value) : "K" (reg));						\
		__value; })


/* TBL write */
#define tlb_commit() asm volatile ("tlbwi")

struct vm_cpu_;

extern void coproc_write(const struct vm_cpu_ *, uint32_t coproc, uint32_t code, uint32_t reg);
extern uint32_t coproc_read(const struct vm_cpu_ *, uint32_t coproc, uint32_t code);

#endif
