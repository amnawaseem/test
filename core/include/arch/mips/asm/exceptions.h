#ifndef _PHIDIAS_MIPS_ASM_EXCEPTIONS_H_
#define _PHIDIAS_MIPS_ASM_EXCEPTIONS_H_

/* EXC codes */
#define EXC_INT		0x00
#define EXC_TLBMOD	0x01
#define EXC_TLBL	0x02
#define	EXC_TLBS	0x03
#define EXC_ADEL	0x04
#define EXC_ADES	0x05
#define EXC_IBE		0x06
#define EXC_DBE		0x07
#define EXC_SYS		0x08
#define EXC_BP		0x09
#define EXC_RI		0x0a
#define EXC_CPU		0x0b
#define	EXC_OVF		0x0c
#define	EXC_TRAP	0x0d
#define EXC_MSAFPE	0x0e
#define EXC_FPE		0x0f
#define EXC_TLBRI	0x13
#define EXC_TLBXI	0x14
#define EXC_GE		0x1b

/* Guest EXC codes */
#define GEXC_GPSI	0x00
#define GEXC_GSFC	0x01
#define GEXC_HC		0x02
#define GEXC_GRR	0x03
#define GEXC_GVA	0x08
#define GEXC_GHFC	0x09
#define GEXC_GPA	0x0A

/* c0 Opcodes */
#define OPC_COP0	0x10
#define OPC_COP1	0x11
#define OPC_COP2	0x12
#define OPC_CACHE	0x2f

#endif /* _PHIDIAS_MIPS_ASM_EXCEPTIONS_H_ */
