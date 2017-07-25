#include <phidias.h>
#include <vm.h>
#include <emulate/core.h>
#include <emulate/uart.h>
#include <asm/uart-pic32.h>
#include <arch/asm/offsets.h>

void emulate_uart_pic32_initialize(const vm_cpu *vm_cpu_periph, emulate_uart_pic32 *pic32) {
	(void)vm_cpu_periph;
	pic32->regs[0] = 0;
	pic32->regs[1] = PIC32BIT_UXURXEN | PIC32BIT_UXUTXEN | PIC32BIT_UXTRMT;
	pic32->regs[2] = 0;
	pic32->regs[3] = 0;
	pic32->regs[4] = 0;
	pic32->buf_cnt = 0;
}

uint32_t emulate_uart_pic32_load(const vm_cpu *vm_cpu_periph, emulate_uart_pic32 *pic32, uint32_t bar, uintptr_t offset) {
	(void)vm_cpu_periph;
	(void)bar;

	switch (offset) {
	case PIC32REG_UXSTA:
		//printf("UartPic32Reg R    UXSTA    , val %08x\r\n", pic32->regs[1]);
		return pic32->regs[1];
	default:
		printf("UartPic32Reg R off %08x\r\n", offset);
		return 0;
	}
}

void emulate_uart_pic32_store(const vm_cpu *vm_cpu_periph, emulate_uart_pic32 *pic32, uint32_t bar, uintptr_t offset, uint32_t value) {
	(void)vm_cpu_periph;
	(void)bar;

	switch (offset) {
	case PIC32REG_UXMODE:
	case PIC32REG_UXMODE + CLR_OFFSET:
	case PIC32REG_UXMODE + SET_OFFSET:
	case PIC32REG_UXMODE + INV_OFFSET:
	case PIC32REG_UXSTA:
	case PIC32REG_UXSTA + CLR_OFFSET:
	case PIC32REG_UXSTA + SET_OFFSET:
	case PIC32REG_UXSTA + INV_OFFSET:
		break;
	case PIC32REG_UXTXREG:
#ifdef FEATURE_MULTIPLEXER
		mux_out_char(value);
#else
		pic32->buffer[pic32->buf_cnt++] = value;
		//printf("UartPic32Reg W   UXTXREG   , val %02x\r\n", value);
		if (value == '\n'){
			printf("Guest: %s", pic32->buffer);
			pic32->buf_cnt = 0;
		}
#endif
		break;
	default:
		printf("UartPic32Reg W off %08x, val %08x\r\n", offset, value);
	}
}
