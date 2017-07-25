#ifndef _PHIDIAS_MIPS_HYPCALL_
#define _PHIDIAS_MIPS_HYPCALL_

void vm_cpu_hypcall( void );

#define HYPCALL_ID_REGISTER_IRQ	0x1
#define HYPCALL_ID_ACK_IRQ	0x2
#define HYPCALL_ID_TRIGGER_IRQ	0x3

#endif /* _PHIDIAS_MIPS_HYPCALL_ */
