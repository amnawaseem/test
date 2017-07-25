#ifndef _PHIDIAS_MIPS_EXCEPTIONS_H_
#define _PHIDIAS_MIPS_EXCEPTIONS_H_

void general_exception_handler(void);
void special_interrrupt_handler(void);
void tlb_miss_handler(void);
void hypervisor_exception_handler(unsigned int);

extern uint32_t perfcnt0;
extern uint32_t perfcnt1;

#endif /* _PHIDIAS_MIPS_EXCEPTIONS_H_ */
