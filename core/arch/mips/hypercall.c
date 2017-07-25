#include <phidias.h>
#include <schedule.h>
#include <mmio.h>
#include <vm.h>
#include <arch/vm.h>
#include <arch/cpu_state.h>
#include <arch/cpu_ops.h>
#include <arch/hypercall.h>
#include <drivers/irq-pic32.h>
#include <arch/exceptions.h>

void vm_cpu_hypcall() {
	/* a0 contains code */
	uint32_t hyp_code = rfpss(4);
	uint32_t irq_id	= rfpss(5);
	uint32_t offset = rfpss(6);
	uint32_t prio = rfpss(7);
	uint32_t guest = getRootGuestID();
	switch (hyp_code){
		case HYPCALL_ID_REGISTER_IRQ:
			/* register IRQ: 
			 * a1 - irq_id
			 * a2 - offset (to guests' EBASE)
			 * a3 - priority (up to 6) */
			if (irq_id > 256){
				printf("irq: bad ID %d from Guest %d\r\n", irq_id, guest);
				break;
			}

			if (offset & 0xFFFF0000){
				printf("irq: bad offset %08x from Guest %d\r\n", offset, guest);
				break;
			}

			/* prio 7 reserved for phidias */
			if (prio > 6){
				printf("irq: bad prio %d from Guest %d\r\n", prio, guest);
				break;
			}

			/* test if IRQ is taken */
			if ( _specification.irq_owner_table[irq_id] != current_vm_cpu->vm){
				printf("irq: Guest %d tried to map foreign IRQ\r\n", guest);
				break;
			}

			/* args reasonable, register it */
			irq_pic32_enable_irq_vm(core_memarea(MEMAREA_IRQ_PIC32), irq_id, prio, offset, guest);
			printf("Guest %d registers INT %d, prio %d with OFF 0x%08x\r\n", guest, irq_id, prio, offset);
		break;
		
		case HYPCALL_ID_ACK_IRQ:
			printf("Guest %d wanted to ACK an INT but this is unimplemented\r\n", guest);
		break;
		
		case HYPCALL_ID_TRIGGER_IRQ:
			/* trigger IRQ - set its flag
			 * $a1 - irq_id */
			if ( _specification.irq_owner_table[irq_id] != current_vm_cpu->vm){
				printf("irq: Guest %d wanted to trigger foreign IRQ %d\r\n", guest, irq_id);
				break;
			}
			
			/* guests IRQ, trigger it */
			setGuestID(guest);
			mmio_write32(0xbf810040 + ((irq_id/32) * 0x10) + 0x8, 1 << (irq_id % 32));
			setGuestID(0);
		break;
		
		default:
			printf("perfcnt0: %08x perfcnt1: %08x\r\n", perfcnt0, perfcnt1);
			printf("Hypcall with unknown code: %08x\r\n$a1: %08x\t$a2: %08x\r\n", hyp_code, irq_id, offset);
		break;
	}
	advance_epc(4);
	upcall();
}
