#include <phidias.h>
#include <vm.h>
#include <interrupts.h>
#include <emulate/core.h>
#include <emulate/uart.h>
#include <emulate/irq.h>
#include <arch/virt-vmx.h>
#include <arch/asm/vmx_regs.h>

#define	EM_UART_IRQ	0x51

void emulate_uart_16550_initialize(const vm_cpu *vm_cpu_periph, emulate_uart_16550 *u16550) {
	(void)vm_cpu_periph; (void)u16550;
}

uint32_t emulate_uart_16550_load(const vm_cpu *vm_cpu_periph, emulate_uart_16550 *u16550, uint32_t bar, uintptr_t offset) {
	uint32_t value;

	(void)vm_cpu_periph; (void)bar;

	/*if (offset != 5)
		printf("UART R %x\r\n", offset);*/

	switch (offset) {
	case 0x01:
		return u16550->regs[1];
	case 0x02:
		if ((u16550->fifo.count > 0) && (u16550->regs[1] & 0x01))
			return 0x04;
		if (u16550->regs[1] & 0x02)
			return 0x02;
		return 0x01;
	case 0x03:
		return u16550->regs[3];
	case 0x04:
		return u16550->regs[4];
	case 0x05:
		return 0x60 | ((u16550->fifo.count > 0) ? 0x01 : 0x00);
	case 0x06:
		return 0x00;
	case 0x07:
		return u16550->regs[7];
	default:
		if (u16550->fifo.count == 0)
			return 0xff;
		value = uartfifo_pop(&u16550->fifo);
		if ((u16550->regs[1] & 0x01) && (u16550->fifo.count > 0))
			deliver_irq_to_vm(vm_cpu_periph->vm, EM_UART_IRQ, 0);
		return value;
	}
}

void emulate_uart_16550_store(const vm_cpu *vm_cpu_periph, emulate_uart_16550 *u16550,
			uint32_t bar, uintptr_t offset, uint32_t value) {
	(void)vm_cpu_periph; (void)u16550; (void)bar;

	value &= 0xff;

	/*if (offset != 0)
		printf("UART W %x %x (@%x)\r\n", offset, value, vmread(VMXREG_NWG_RIP));*/

	switch (offset) {
	case 0x00:
		if (u16550->regs[3] & 0x80) break;
#ifdef FEATURE_MULTIPLEXER
		mux_out_char(value,vm_cpu_periph->pagetable_identifier);
#else
		printf("\e[%dm%c\e[0m", 30 + vm_cpu_periph->pagetable_identifier, value);
#endif
		break;
	case 0x01:
		if (u16550->regs[3] & 0x80) break;
		u16550->regs[1] = value;
		if (u16550->regs[1] & 0x02)
			deliver_irq_to_vm(vm_cpu_periph->vm, EM_UART_IRQ, 0);
		else if ((u16550->regs[1] & 0x01) && (u16550->fifo.count > 0))
			deliver_irq_to_vm(vm_cpu_periph->vm, EM_UART_IRQ, 0);
		break;
	case 0x02:
		u16550->regs[2] = value;
		break;
	case 0x03:
		u16550->regs[3] = value;
		break;
	case 0x04:
		u16550->regs[4] = value;
		break;
	case 0x07:
		u16550->regs[7] = value;
		break;
	default:
		return;
	}
}

void emulate_uart_16550_push_character(const vm_cpu *vm_cpu_periph, emulate_uart_16550 *u16550, uint8_t ch) {
	if (u16550->fifo.count == UARTFIFO_SIZE) {
		printf("UART FIFO OVERRUN\r\n");
		return;
	}

	uartfifo_push(&u16550->fifo, ch);

	if (u16550->regs[1] & 0x01)
		deliver_irq_to_vm(vm_cpu_periph->vm, EM_UART_IRQ, 0);
}
