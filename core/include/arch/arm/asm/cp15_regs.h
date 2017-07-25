#ifndef _PHIDIAS_ARCH_ARM_ASM_CP15_REGS_H_
#define	_PHIDIAS_ARCH_ARM_ASM_CP15_REGS_H_

// CP15 registers, ordered by encoding

# define	FPSID(reg)	p10, 7, reg,  c0,  c0, 0
# define	MVFR1(reg)	p10, 7, reg,  c6,  c0, 0
# define	MVFR0(reg)	p10, 7, reg,  c7,  c0, 0
# define	FPEXC(reg)	p10, 7, reg,  c8,  c0, 0

# define	MIDR(reg)	p15, 0, reg,  c0,  c0, 0
# define	CTR(reg)	p15, 0, reg,  c0,  c0, 1
# define	MPIDR(reg)	p15, 0, reg,  c0,  c0, 5
# define	ID_PFR0(reg)	p15, 0, reg,  c0,  c1, 0
# define	ID_PFR1(reg)	p15, 0, reg,  c0,  c1, 1
# define	ID_PFR2(reg)	p15, 0, reg,  c0,  c1, 2
# define	ID_PFR3(reg)	p15, 0, reg,  c0,  c1, 3
# define	ID_MMFR0(reg)	p15, 0, reg,  c0,  c1, 4
# define	ID_MMFR1(reg)	p15, 0, reg,  c0,  c1, 5
# define	ID_MMFR2(reg)	p15, 0, reg,  c0,  c1, 6
# define	ID_MMFR3(reg)	p15, 0, reg,  c0,  c1, 7
# define	ID_ISAR0(reg)	p15, 0, reg,  c0,  c2, 0
# define	ID_ISAR1(reg)	p15, 0, reg,  c0,  c2, 1
# define	ID_ISAR2(reg)	p15, 0, reg,  c0,  c2, 2
# define	ID_ISAR3(reg)	p15, 0, reg,  c0,  c2, 3
# define	ID_ISAR4(reg)	p15, 0, reg,  c0,  c2, 4
# define	ID_ISAR5(reg)	p15, 0, reg,  c0,  c2, 5
# define	SCTLR(reg)	p15, 0, reg,  c1,  c0, 0
# define	CPACR(reg)	p15, 0, reg,  c1,  c0, 2
# define	SCR(reg)	p15, 0, reg,  c1,  c1, 0
# define	NSACR(reg)	p15, 0, reg,  c1,  c1, 2
# define	TTBR0(reg)	p15, 0, reg,  c2,  c0, 0
# define	TTBR1(reg)	p15, 0, reg,  c2,  c0, 1
# define	TTBCR(reg)	p15, 0, reg,  c2,  c0, 2
# define	DACR(reg)	p15, 0, reg,  c3,  c0, 0
# define	DFSR(reg)	p15, 0, reg,  c5,  c0, 0
# define	IFSR(reg)	p15, 0, reg,  c5,  c0, 1
# define	DFAR(reg)	p15, 0, reg,  c6,  c0, 0
# define	IFAR(reg)	p15, 0, reg,  c6,  c0, 2
# define	DRBAR(reg)	p15, 0, reg,  c6,  c1, 0
# define	DRSR(reg)	p15, 0, reg,  c6,  c1, 2
# define	DRACR(reg)	p15, 0, reg,  c6,  c1, 4
# define	RGNR(reg)	p15, 0, reg,  c6,  c2, 0
# define	BPIALLIS	p15, 0,  r0,  c7,  c1, 6
# define	ICIALLU		p15, 0,  r0,  c7,  c5, 0
# define	ICIMVAU		p15, 0,  r0,  c7,  c5, 1
# define	BPIALL		p15, 0,  r0,  c7,  c5, 6
# define	DCIMVAC(reg)	p15, 0, reg,  c7,  c6, 1
# define	DCISW(reg)	p15, 0, reg,  c7,  c6, 2
# define	DCCMVAC(reg)	p15, 0, reg,  c7, c10, 1
# define	DCCSW(reg)	p15, 0, reg,  c7, c10, 2
# define	DCCMVAU(reg)	p15, 0, reg,  c7, c11, 1
# define	DCCIMVAC(reg)	p15, 0, reg,  c7, c14, 1
# define	DCCISW(reg)	p15, 0, reg,  c7, c14, 2
# define	TLBIALLIS	p15, 0,  r0,  c8,  c3, 0
# define	TLBIMVAIS(reg)	p15, 0, reg,  c8,  c3, 1
# define	TLBIALL		p15, 0,  r0,  c8,  c7, 0
# define	TLBIMVA(reg)	p15, 0, reg,  c8,  c7, 1
# define	TLBIASID(reg)	p15, 0, reg,  c8,  c7, 2
# define	PRRR(reg)	p15, 0, reg, c10,  c2, 0
# define	NMRR(reg)	p15, 0, reg, c10,  c2, 1
# define	VBAR(reg)	p15, 0, reg, c12,  c0, 0
# define	MVBAR(reg)	p15, 0, reg, c12,  c0, 1
# define	CONTEXTIDR(reg)	p15, 0, reg, c13,  c0, 1
# define	TPIDRURW(reg)	p15, 0, reg, c13,  c0, 2
# define	TPIDRURO(reg)	p15, 0, reg, c13,  c0, 3
# define	TPIDRPRW(reg)	p15, 0, reg, c13,  c0, 4
# define	CCSIDR(reg)	p15, 1, reg,  c0,  c0, 0
# define	CLIDR(reg)	p15, 1, reg,  c0,  c0, 1
# define	CSSELR(reg)	p15, 2, reg,  c0,  c0, 0

// cp10
# define	COPROC_CODE_FPSID	0xa7000
# define	COPROC_CODE_MVFR1	0xa7600
# define	COPROC_CODE_MVFR0	0xa7700
# define	COPROC_CODE_FPEXC	0xa7800

// cp15
# define	COPROC_CODE_MIDR	0xf0000
# define	COPROC_CODE_CTR		0xf0001
# define	COPROC_CODE_MPIDR	0xf0005
# define	COPROC_CODE_ID_PFR0	0xf0010
# define	COPROC_CODE_ID_PFR1	0xf0011
# define	COPROC_CODE_ID_PFR2	0xf0012
# define	COPROC_CODE_ID_PFR3	0xf0013
# define	COPROC_CODE_ID_MMFR0	0xf0014
# define	COPROC_CODE_ID_MMFR1	0xf0015
# define	COPROC_CODE_ID_MMFR2	0xf0016
# define	COPROC_CODE_ID_MMFR3	0xf0017
# define	COPROC_CODE_ID_ISAR0	0xf0020
# define	COPROC_CODE_ID_ISAR1	0xf0021
# define	COPROC_CODE_ID_ISAR2	0xf0022
# define	COPROC_CODE_ID_ISAR3	0xf0023
# define	COPROC_CODE_ID_ISAR4	0xf0024
# define	COPROC_CODE_ID_ISAR5	0xf0025
# define	COPROC_CODE_SCTLR	0xf0100
# define	COPROC_CODE_CPACR	0xf0102
# define	COPROC_CODE_NSACR	0xf0112
# define	COPROC_CODE_TTBR0	0xf0200
# define	COPROC_CODE_TTBR1	0xf0201
# define	COPROC_CODE_TTBCR	0xf0202
# define	COPROC_CODE_DACR	0xf0300
# define	COPROC_CODE_DFSR	0xf0500
# define	COPROC_CODE_IFSR	0xf0501
# define	COPROC_CODE_DFAR	0xf0600
# define	COPROC_CODE_IFAR	0xf0602
# define	COPROC_CODE_BPIALLIS	0xf0716
# define	COPROC_CODE_ICIALLU	0xf0750
# define	COPROC_CODE_ICIMVAU	0xf0751
# define	COPROC_CODE_BPIALL	0xf0756
# define	COPROC_CODE_DCIMVAC	0xf0761
# define	COPROC_CODE_DCISW	0xf0762
# define	COPROC_CODE_DCCMVAC	0xf07a1
# define	COPROC_CODE_DCCSW	0xf07a2
# define	COPROC_CODE_DCCMVAU	0xf07b1
# define	COPROC_CODE_DCCIMVAC	0xf07e1
# define	COPROC_CODE_DCCISW	0xf07e2
# define	COPROC_CODE_TLBIALLIS	0xf0830
# define	COPROC_CODE_TLBIALL	0xf0870
# define	COPROC_CODE_TLBIMVA	0xf0871
# define	COPROC_CODE_TLBIASID	0xf0872
# define	COPROC_CODE_TLBIMVAA	0xf0873
# define	COPROC_CODE_PRRR	0xf0a20
# define	COPROC_CODE_NMRR	0xf0a21
# define	COPROC_CODE_CONTEXTIDR	0xf0d01
# define	COPROC_CODE_TPIDRURW	0xf0d02
# define	COPROC_CODE_TPIDRURO	0xf0d03
# define	COPROC_CODE_TPIDRPRW	0xf0d04
# define	COPROC_CODE_CCSIDR	0xf1000
# define	COPROC_CODE_CLIDR	0xf1001
# define	COPROC_CODE_CSSELR	0xf2000

# define	COPROC_CODE_CNTHCTL	0xf4e10
# define	COPROC_CODE_CNTHP_TVAL	0xf4e20
# define	COPROC_CODE_CNTHP_CTL	0xf4e21
# define	COPROC_CODE64_CNTPCT	0xf0e

# define	COPROC_SPECIFIC_CODE_MPCORE_CBAR	0xf4f00

// cp15 register bit definitions (same order), also accessible from C code

# define	SCR_NS_BIT		0x00000001
# define	SCTLR_BIT_M		0x00000001
# define	SCTLR_BIT_V		0x00002000
# define	TTBR0_BIT_S		0x00000002
# define	TTBR0_BIT_INNER_C_WBWA	0x00000040
# define	TTBR0_BIT_OUTER_C_WBWA	0x00000008
# define	TTBCR_BIT_EAE		0x80000000
# define	DACR_CLIENT(dom)	(0x1 << (dom*2))
# define	FSR_BIT_WRITE		0x00000800

// constants used with cp15 registers

# define	CONTEXTIDR_ASID_RESERVED	0
# define	CONTEXTIDR_ASID_HOST_PAGING	1

// VFP register bits

# define	FPEXC_EX		0x80000000
# define	FPEXC_EN		0x40000000

#endif
