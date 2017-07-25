#ifndef	_PHIDIAS_ARCH_MIPS_ASM_CP0_REGS_H_
#define	_PHIDIAS_ARCH_MIPS_ASM_CP0_REGS_H_

#ifndef __ASSEMBLER__

# define	c0_index		 0, 0
# define	c0_random		 1, 0
# define	c0_entrylo0		 2, 0
# define	c0_entrylo1		 3, 0
# define	c0_context		 4, 0
# define	c0_pagemask		 5, 0
# define	c0_pagegrain		 5, 1
# define	c0_segctl0		 5, 2
# define	c0_segctl1		 5, 3
# define	c0_segctl2		 5, 4
# define	c0_wired		 6, 0
# define	c0_badvaddr		 8, 0
# define	c0_badinstr		 8, 1
# define	c0_badinstrp		 8, 2
# define	c0_count		 9, 0
# define	c0_entryhi		10, 0
# define	c0_guestctl1		10, 4
# define	c0_guestctl2		10, 5
# define	c0_guestctl3		10, 6
# define	c0_compare		11, 0
# define	c0_guestctl0ext		11, 4
# define	c0_status		12, 0
# define	c0_intctl		12, 1
# define	c0_srsctl		12, 2
# define	c0_srsmap		12, 3
# define	c0_guestctl0		12, 6
# define	c0_gtoffset		12, 7
# define	c0_cause		13, 0
# define	c0_nestedexc		13, 5
# define	c0_epc			14, 0
# define	c0_ebase		15, 1
# define	c0_config0		16, 0
# define	c0_config1		16, 1
# define	c0_config2		16, 2
# define	c0_config3		16, 3
# define	c0_config4		16, 4
# define	c0_config5		16, 5
# define	c0_config6		16, 6
# define	c0_watchlo		18, 0
# define	c0_watchhi		19, 0
# define	c0_perfctl0		25, 0
# define	c0_perfcnt0		25, 1
# define	c0_perfctl1		25, 2
# define	c0_perfcnt1		25, 3
# define	c0_taglo		28, 0
# define	c0_datalo		28, 1
# define	c0_taghi		29, 0
# define	c0_datahi		29, 1
# define	c0_errorepc		30, 0
# define	c0_kscratch2		31, 2
# define	c0_kscratch3		31, 3
# define	c0_kscratch4		31, 4
# define	c0_kscratch5		31, 5
# define	c0_kscratch6		31, 6
# define	c0_kscratch7		31, 7

#else  /* __ASSEMBLER__ */
# define	MIPS_CREG(a,b)		$a, b

# define	c0_index		MIPS_CREG( 0, 0)
# define	c0_random		MIPS_CREG( 1, 0)
# define	c0_entrylo0		MIPS_CREG( 2, 0)
# define	c0_entrylo1		MIPS_CREG( 3, 0)
# define	c0_context		MIPS_CREG( 4, 0)
# define	c0_pagemask		MIPS_CREG( 5, 0)
# define	c0_pagegrain		MIPS_CREG( 5, 1)
# define	c0_segctl0		MIPS_CREG( 5, 2)
# define	c0_segctl1		MIPS_CREG( 5, 3)
# define	c0_segctl2		MIPS_CREG( 5, 4)
# define	c0_wired		MIPS_CREG( 6, 0)
# define	c0_badvaddr		MIPS_CREG( 8, 0)
# define	c0_count		MIPS_CREG( 9, 0)
# define	c0_entryhi		MIPS_CREG(10, 0)
# define	c0_guestctl1		MIPS_CREG(10, 4)
# define	c0_guestctl2		MIPS_CREG(10, 5)
# define	c0_guestctl3		MIPS_CREG(10, 6)
# define	c0_compare		MIPS_CREG(11, 0)
# define	c0_guestctl0ext		MIPS_CREG(11, 4)
# define	c0_status		MIPS_CREG(12, 0)
# define	c0_intctl		MIPS_CREG(12, 1)
# define	c0_srsctl		MIPS_CREG(12, 2)
# define	c0_srsmap		MIPS_CREG(12, 3)
# define	c0_guestctl0		MIPS_CREG(12, 6)
# define	c0_gtoffset		MIPS_CREG(12, 7)
# define	c0_cause		MIPS_CREG(13, 0)
# define	c0_nestedexc		MIPS_CREG(13, 5)
# define	c0_epc			MIPS_CREG(14, 0)
# define	c0_ebase		MIPS_CREG(15, 1)
# define	c0_config0		MIPS_CREG(16, 0)
# define	c0_config1		MIPS_CREG(16, 1)
# define	c0_config2		MIPS_CREG(16, 2)
# define	c0_config3		MIPS_CREG(16, 3)
# define	c0_config4		MIPS_CREG(16, 4)
# define	c0_config5		MIPS_CREG(16, 5)
# define	c0_config6		MIPS_CREG(16, 6)
# define	c0_watchlo		MIPS_CREG(18, 0)
# define	c0_watchhi		MIPS_CREG(19, 0)
# define	c0_perfctl0		MIPS_CREG(25, 0)
# define	c0_perfcnt0		MIPS_CREG(25, 1)
# define	c0_perfctl1		MIPS_CREG(25, 2)
# define	c0_perfcnt1		MIPS_CREG(25, 3)
# define	c0_taglo		MIPS_CREG(28, 0)
# define	c0_datalo		MIPS_CREG(28, 1)
# define	c0_taghi		MIPS_CREG(29, 0)
# define	c0_datahi		MIPS_CREG(29, 1)
# define	c0_errorepc		MIPS_CREG(30, 0)
# define	c0_kscratch2		MIPS_CREG(31, 2)
# define	c0_kscratch3		MIPS_CREG(31, 3)
# define	c0_kscratch4		MIPS_CREG(31, 4)
# define	c0_kscratch5		MIPS_CREG(31, 5)
# define	c0_kscratch6		MIPS_CREG(31, 6)
# define	c0_kscratch7		MIPS_CREG(31, 7)

#endif /* ASSEMBLER */


#endif
