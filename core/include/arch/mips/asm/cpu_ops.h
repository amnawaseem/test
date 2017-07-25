#ifndef	_PHIDIAS_ARCH_MIPS_ASM_CPU_OPS_H_
#define	_PHIDIAS_ARCH_MIPS_ASM_CPU_OPS_H_

/* arch-specific cpu defines */
#define TLB_ALL_ADDRESSES	~0U

/* MIPS OpCodes */
#define OPC_COP0	0x10
#define OPC_LW		0x23
#define OPC_LHU		0x25
#define OPC_SB		0x28
#define OPC_SH		0x29
#define OPC_SW		0x2B

/* COP0 SubCodes */
#define COP0_MT		0x04
#endif
