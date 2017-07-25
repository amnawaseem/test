#ifndef	_PHIDIAS_DRIVERS_UART_PIC32_H_
#define	_PHIDIAS_DRIVERS_UART_PIC32_H_

# include <asm/uart-pic32.h>

struct memarea_;

void uart_pic32_setup(const struct memarea_ *area);
void uart_pic32_putchar(const struct memarea_ *area, uint8_t ch);
void uart_pic32_putstring(const struct memarea_ *area, const char *string);
uint32_t uart_pic32_irq(const struct memarea_ *area);

#endif

