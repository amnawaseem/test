#include <phidias.h>
#include <debug.h>
#include <mmio.h>
#include <vm.h>
#include <schedule.h>
#include <arch/exceptions.h>
#include <arch/asm/exceptions.h>
#include <emulate/core.h>
#include <arch/cpu_state.h>
#include <arch/cpu_ops.h>
#include <arch/vm.h>
#include <arch/tlb.h>
#include <arch/hypercall.h>
#include <drivers/irq-pic32.h>

uint32_t perfcnt1 = 0;
uint32_t perfcnt0 = 0;


void __attribute__ ((section (".isr"), noinline )) general_exception_handler(void){
	unsigned int _excep_code;
	unsigned int _excep_addr;

	uint32_t intstat;
	uint8_t	 irq_id, irq_prio;
	uint16_t offset;

	_excep_code = mfc0(c0_cause);
	_excep_addr = mfc0(c0_epc);

	_excep_code = (_excep_code & 0x0000007C) >> 2;

	switch (_excep_code){
		case EXC_GE:	/* hypervisor excep */
			hypervisor_exception_handler(_excep_addr);
			break;
		case EXC_TLBL:
			printf("TLBL Exception: ");
			vm_cpu_dump_state(current_vm_cpu);
			while(1);
			tlb_miss_handler();
			break;
		case EXC_TLBS:
			printf("TLBS Exception: ");
			tlb_miss_handler();
			break;
		case EXC_INT:
			perfcnt0	= mfc0(c0_perfcnt0);
			perfcnt1	= mfc0(c0_perfcnt0);
			intstat		= mmio_read32(0xbf810020);
			irq_id		= intstat & 0xFF;
			irq_prio	= (intstat >> 8) & 0x7;
			offset		= mmio_read32(0xbf810540 + 4*irq_id);

			/* ACK the INT */
			irq_pic32_ack_irq(core_memarea(MEMAREA_IRQ_PIC32), irq_id);

			/* inject the INT */
/*			printf("guests' SRSCTL: %08x\r\n", mfgc0(c0_srsctl));
			printf("GuestCtl0: %08x\r\n", mfc0(c0_guestctl0));
			printf("GuestCtl1: %08x\r\n", mfc0(c0_guestctl1));
			printf("GuestCtl2: %08x\r\n", mfc0(c0_guestctl2));
			printf("GuestCtl3: %08x\r\n", mfc0(c0_guestctl3));
			printf("GuestCtl0Ext: %08x\r\n", mfc0(c0_guestctl0ext));
			printf("PRISS: %08x\r\n", mmio_read32(0xBF810010));
			printf("INTSTAT: %08x\r\n", intstat);
*/
			mtc0(c0_guestctl2, irq_prio << 24 | 1 << 18 | offset);
			vm_cpu_upcall();
			break;
		default:
			printf("Phidias Exception Code: %02x\n\r", _excep_code);
			printf("Phidias Exception Address: %08x\n\r", _excep_addr);
			printf("Phidias Exception BadVAddr: %08x\r\n", mfc0(c0_badvaddr));
			vm_cpu_dump_state(current_vm_cpu);
			while(1);
			break;
	}
	while(1);
}

void __attribute__ ((section (".isr"), noinline )) special_interrupt_handler(void){
	uint32_t intstat, irq_id, irq_prio, div, mod, offset;

	intstat		= mmio_read32(0xbf810020);
	irq_id		= intstat & 0xFF;
	irq_prio	= (intstat >> 8) & 0x7;
	offset		= mmio_read32(0xbf810540 + 4*irq_id);

	/* ACK the INT */
	div = irq_id / 0x20;
	mod = irq_id % 0x20;
	mmio_write32(0xbf810044 + (0x10 * div), 1 << mod);

	/* inject the INT */
	mtc0(c0_guestctl2, irq_prio << 24 | 1 << 18 | offset);
	asm volatile ("eret");
}

void __attribute__ ((section(".isr"), noinline )) tlb_miss_handler(void){
	unsigned int _tlb_epc;
	unsigned int _tlb_bad_addr;

	const emulate *emul;

	_tlb_bad_addr	= mfc0(c0_badvaddr);
	_tlb_epc	= mfc0(c0_epc);

	const memarea *mem = find_memarea_by_va(current_vm_cpu->memarea_tree, _tlb_bad_addr);

	if( mem != NULL){
		/* add entry in TLB */
		write_tlb_entry(mem, _tlb_bad_addr);
	}
	else{
		emul = find_emulate_by_va(current_vm_cpu->emulate_tree, _tlb_bad_addr);
		if (emul !=  NULL){
			vm_cpu_emulate_faulted(current_vm_cpu, emul, _tlb_bad_addr);
			upcall();
		}
		else{
			printf("Unmapped BadVAddr: %08x @Address: %08x\r\n", _tlb_bad_addr, _tlb_epc);
			vm_cpu_dump_state(current_vm_cpu);
			while(1);
		}
	}
}

void __attribute__ ((section (".isr"), noinline)) hypervisor_exception_handler (unsigned int addr){
	unsigned int _guest_ctl, _guest_exc_code;
	uint32_t _bad_instr, value;
	uint8_t src_reg, dest_reg, dest_sel;
	vm_cpu_state *state = current_vm_cpu->cpu_state;

	_guest_ctl = mfc0(c0_guestctl0);
	_guest_exc_code = ((_guest_ctl & 0x7C) >> 2);

	switch (_guest_exc_code){
		case GEXC_GPSI:		/* guest privileged instr. */
			_bad_instr = mfc0(c0_badinstr);
			switch (_bad_instr >> 26){
				case OPC_COP0:
					/* m[tf]c0 */
					switch ((_bad_instr >> 21) & 0x1f){
						case 0:
							/* move from c0, for now ignore. just add 4 to EPC */
							printf("Guest mfc0'ed @ %08x\r\n", addr);
							advance_epc(4);
							upcall();
						break;
						case 4:
							/* move to c0 */
							switch ((_bad_instr >> 11) & 0x1f){
								/* target reg in CP0 */
								case 9:
									/* count or reserved */
									if( (_bad_instr & 0x7) == 0){
										/* sel = 0, this is count -> we need to adjust GTOffset
										 * to desired Value of Guest.count (in rt) - Root.Count */
										int32_t guest_count = rfpss((_bad_instr >> 16) & 0x1f);
										int32_t root_count = mfc0(c0_count);
										int32_t root_GTOffset = guest_count - root_count;
										mtc0(c0_gtoffset, root_GTOffset);
									}
									else{
										printf("Guest mtc0'ed @ %08x\r\n", addr);
									}
								break;
								default:
									printf("Guest mtc0'ed %08x to Reg %d, Sel %d @ %08x\r\n", rfpss((_bad_instr >> 16) & 0x1f), (_bad_instr >> 11) & 0x1f, _bad_instr &0x7, addr);
								break;
							}
							advance_epc(4);
							upcall();
						break;
						case 14:
							/* WRPGPR: guest has no shadow regs, let him do it */
							printf("Guest WRPGPR'ed @ %08x\r\n", addr);
							advance_epc(4);
							upcall();
						break;
						default:
							printf("uncaught case of OPC_COP0: %08x @ %08x\r\n", _bad_instr, addr);
						break;
					}
				break;
				case OPC_COP1:
					printf("uncaught case of OPC_COP1 @ %08x\r\n", _bad_instr);
				break;
				case OPC_COP2:
					printf("uncaught case of OPC_COP2 @ %08x\r\n", _bad_instr);
				break;
				case OPC_CACHE:
					printf("Guest CACHE'd stuff: %08x @ %08x\r\n", _bad_instr, addr);
					advance_epc(8);
					upcall();
				break;
				default:
					printf("uncaught case of GPSI: %08x @ %08x\r\n", _bad_instr, addr);
				break;
			}
		break;
		case GEXC_GHFC:
			/* guest hardware field change (EXL toggle or TS set) */
			printf("GHFC @ %08x\r\n", addr, state->pc);
			printf("Guest.Status: %08x\r\n", mfgc0(c0_status));
			printf("Guest.EPC: %08x\r\n", mfgc0(c0_epc));
			printf("Guest.Cause: %08x\r\n", mfgc0(c0_cause));
			printf("Guest.BadVAddr: %08x\r\n", mfgc0(c0_badvaddr));
			upcall();
		break;
		case GEXC_GSFC:
			/* Guest software field Change */
			_bad_instr = mfc0(c0_badinstr);
			if (_bad_instr >> 26 == OPC_COP0){
				/*m[ft]c0 */
				if ( ((_bad_instr >> 21) & 0x1f) == COP0_MT){
					/* mtc0 */
					src_reg		= (uint8_t)((_bad_instr >> 16) & 0x1f);
					dest_reg	= (uint8_t)((_bad_instr >> 11) & 0x1f);
					dest_sel	= (uint8_t)(_bad_instr & 0x7);
					value		= rfpss(src_reg);
					printf("MTC0 by Guest %d: %08x into %d, %d\r\n", getRootGuestID(), value, dest_reg, dest_sel);
					if (value == 0x25000000)
						mtgc0(c0_status, 0x35000001);
					/* instr has not been carried out! */
				}
			}
			else {
				printf("GSFC @ %08x: %08x\r\n", addr, _bad_instr);
			}
			advance_epc(4);
			upcall();
		break;
		case GEXC_HC:
			/* Hypcall */
			vm_cpu_hypcall();
		break;
		case GEXC_GRR:
			/* Guest Reserved Instruction */
			printf("GRR @ %08x, cmd: %08x\r\n", mfc0(c0_badinstr), mfc0(c0_badinstrp));
			vm_cpu_dump_state(current_vm_cpu);
		break;
		default:
			printf("Hypervisor Exception Code: %08x @ %08x\r\n", _guest_exc_code, addr);
			vm_cpu_dump_state(current_vm_cpu);
		break;
	}
}
