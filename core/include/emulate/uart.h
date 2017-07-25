#ifndef	_PHIDIAS_EMULATE_UART_H_
#define	_PHIDIAS_EMULATE_UART_H_

# include <vm.h>
# include <emulate/core.h>
# include <emulate/uartfifo.h>

#ifdef EMULATE_UART_PL011
struct emulate_uart_pl011_ {
	uint32_t	irq_status;
	uint32_t	irq_mask;
	uint32_t	interrupt_line;
	struct emulate_uartfifo_	fifo;
};
typedef struct emulate_uart_pl011_	emulate_uart_pl011;

extern void emulate_uart_pl011_initialize(const struct vm_cpu_ *vm_cpu_em, emulate_uart_pl011 *pl011);
extern void emulate_uart_pl011_store(const struct vm_cpu_ *vm_cpu_em, emulate_uart_pl011 *pl011,
			uint32_t bar, uintptr_t offset, uint32_t value);
extern uint32_t emulate_uart_pl011_load(const struct vm_cpu_ *vm_cpu_em, emulate_uart_pl011 *pl011,
			uint32_t bar, uintptr_t offset);
extern void emulate_uart_pl011_push_character(const struct vm_cpu_ *vm_cpu_em, emulate_uart_pl011 *pl011, uint8_t ch);
#endif /* EMULATE_UART_PL011 */

// --------------------------------------------------------------------------

#ifdef EMULATE_UART_16550
struct emulate_uart_16550_ {
	uint32_t	regs[8];
	struct emulate_uartfifo_	fifo;
	uint32_t	interrupt_line;
};
typedef struct emulate_uart_16550_	emulate_uart_16550;

extern void emulate_uart_16550_initialize(const struct vm_cpu_ *vm_cpu_em, emulate_uart_16550 *u16550);
extern void emulate_uart_16550_store(const struct vm_cpu_ *vm_cpu_em, emulate_uart_16550 *u16550,
			uint32_t bar, uintptr_t offset, uint32_t value);
extern uint32_t emulate_uart_16550_load(const struct vm_cpu_ *vm_cpu_em, emulate_uart_16550 *u16550,
			uint32_t bar, uintptr_t offset);
extern void emulate_uart_16550_push_character(const struct vm_cpu_ *vm_cpu_em, emulate_uart_16550 *u16550, uint8_t ch);
#endif /* EMULATE_UART_16550 */

// --------------------------------------------------------------------------

#ifdef EMULATE_UART_PIC32
struct emulate_uart_pic32_ {
	uint32_t	regs[5];
	char		buffer[0x40];
	short		buf_cnt;
};
typedef struct emulate_uart_pic32_	emulate_uart_pic32;

extern void emulate_uart_pic32_initialize(const struct vm_cpu_ *vm_cpu_em, emulate_uart_pic32 *pic32);
extern void emulate_uart_pic32_store(const struct vm_cpu_ *vm_cpu_em, emulate_uart_pic32 *pic32,
			uint32_t bar, uintptr_t offset, uint32_t value);
extern uint32_t emulate_uart_pic32_load(const struct vm_cpu_ *vm_cpu_em, emulate_uart_pic32 *pic32,
			uint32_t bar, uintptr_t offset);
#endif /* EMULATE_UART_PIC32 */

// --------------------------------------------------------------------------

static inline void emulate_uart_push_character(const struct vm_cpu_ *vm_cpu_em, uint8_t ch) {
	if (vm_cpu_em->uart_emulate == NULL) {
		return;
	}

	switch (vm_cpu_em->uart_emulate->type) {
#ifdef EMULATE_UART_PL011
	case EMULATE_TYPE_UART_PL011:
		emulate_uart_pl011_push_character(vm_cpu_em, vm_cpu_em->uart_emulate->control.uart_pl011, ch);
		break;
#endif
#ifdef EMULATE_UART_16550
	case EMULATE_TYPE_UART_16550:
		emulate_uart_16550_push_character(vm_cpu_em, vm_cpu_em->uart_emulate->control.uart_16550, ch);
		break;
#endif
	default:
		(void)ch;
		printf("No uart push!\r\n");
	}
}

#endif
