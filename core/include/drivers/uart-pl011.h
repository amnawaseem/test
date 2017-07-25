#ifndef	_PHIDIAS_DRIVERS_UART_PL011_H_
#define	_PHIDIAS_DRIVERS_UART_PL011_H_

# include <asm/uart-pl011.h>

struct memarea_;

void uart_pl011_setup(const struct memarea_ *area);
void uart_pl011_putchar(const struct memarea_ *area, uint8_t ch);
void uart_pl011_putstring(const struct memarea_ *area, const char *string);
uint32_t uart_pl011_irq(const struct memarea_ *area);

#endif
