#include <phidias.h>
#include <vm.h>
#include <interrupts.h>
#include <emulate/core.h>
#include <emulate/uart.h>
#include <emulate/irq.h>
#include <asm/uart-pl011.h>
#include <multiplexer.h>

static const uint8_t pl011_id[8] = {
	0x11, 0x10, 0x34, 0x00,
	0x0d, 0xf0, 0x05, 0xb1
};

static void pl011_recompute_irq(const vm_cpu *vm_cpu_periph, emulate_uart_pl011 *pl011) {
	uint32_t irq_masked_status;

	if (pl011->fifo.count == 0) {
		pl011->irq_status &= ~PL011BIT_UARTIRQ_RX;
	} else {
		pl011->irq_status |= PL011BIT_UARTIRQ_RX;
	}

	irq_masked_status = pl011->irq_status & pl011->irq_mask;

	if (irq_masked_status) {
		deliver_irq_to_vm(vm_cpu_periph->vm, pl011->interrupt_line, 0);
	}
}

void emulate_uart_pl011_initialize(const vm_cpu *vm_cpu_periph, emulate_uart_pl011 *pl011) {
	(void)vm_cpu_periph;

	pl011->irq_status = PL011BIT_UARTIRQ_TX;
	pl011->irq_mask = 0;
	pl011->interrupt_line = 0x47;
}

uint32_t emulate_uart_pl011_load(const vm_cpu *vm_cpu_periph, emulate_uart_pl011 *pl011, uint32_t bar, uintptr_t offset) {
	uint32_t value;

	(void)bar;

	switch (offset) {
	case PL011REG_UARTDR:
		value = uartfifo_pop(&pl011->fifo);
		pl011_recompute_irq(vm_cpu_periph, pl011);
		return value;
	case PL011REG_UARTFR:
		value = PL011BIT_UARTFR_TXFE;
		if (pl011->fifo.count == 0)
			value |= PL011BIT_UARTFR_RXFE;
		else if (pl011->fifo.count == UARTFIFO_SIZE)
			value |= PL011BIT_UARTFR_RXFF;
		return value;
	case PL011REG_UARTIBRD:
		return 0x2;
	case PL011REG_UARTFBRD:
		return 0xb;
	case PL011REG_UARTLCR:
		return 0x70;
	case PL011REG_UARTCR:
		return 0x301;
	case PL011REG_UARTIFLS:
		return 0x0;
	case PL011REG_UARTIMSC:
		return pl011->irq_mask;
	case PL011REG_UARTRIS:
		return pl011->irq_status;
	case PL011REG_UARTMIS:
		return pl011->irq_status & pl011->irq_mask;
		break;
	case AMBA_PERIPHID0 ... AMBA_PRIMECELLID3:
		return pl011_id[(offset - AMBA_PERIPHID0) >> 2];
	default:
		printf("UartPl011Reg R %x\r\n", offset);
		return 0;
	}
}

void emulate_uart_pl011_store(const vm_cpu *vm_cpu_periph, emulate_uart_pl011 *pl011, uint32_t bar, uintptr_t offset, uint32_t value) {
	(void)bar;

	switch (offset) {
	case PL011REG_UARTDR:
#ifdef FEATURE_MULTIPLEXER
		mux_out_char(value,vm_cpu_periph->pagetable_identifier);
#endif
		break;
	case PL011REG_UARTIBRD:
	case PL011REG_UARTFBRD:
	case PL011REG_UARTLCR:
	case PL011REG_UARTCR:
	case PL011REG_UARTIFLS:
		break;
	case PL011REG_UARTIMSC:
		pl011->irq_mask = value;
		pl011_recompute_irq(vm_cpu_periph, pl011);
		break;
	case PL011REG_UARTICR:
		pl011->irq_status &= ~value;
		pl011_recompute_irq(vm_cpu_periph, pl011);
		break;
	default:
		printf("UartPl011Reg W %x\r\n", offset);
	}
}

void emulate_uart_pl011_push_character(const vm_cpu *vm_cpu_periph, emulate_uart_pl011 *pl011, uint8_t ch) {
	if (pl011->fifo.count == UARTFIFO_SIZE) {
		printf("UART FIFO OVERRUN\r\n");
		return;
	}

	uartfifo_push(&pl011->fifo, ch);
	pl011_recompute_irq(vm_cpu_periph, pl011);
}
