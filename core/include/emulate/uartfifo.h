#ifndef	_PHIDIAS_EMULATE_UARTFIFO_H_
#define	_PHIDIAS_EMULATE_UARTFIFO_H_

# define	UARTFIFO_SIZE		0x40

struct emulate_uartfifo_ {
	uint8_t		buffer[UARTFIFO_SIZE];
	uint32_t	w_index;
	uint32_t	r_index;
	uint32_t	count;
};
typedef struct emulate_uartfifo_	emulate_uartfifo;

static inline uint8_t uartfifo_pop(emulate_uartfifo *fifo) {
	if (fifo->count > 0) {
		fifo->r_index = (fifo->r_index + 1) & (UARTFIFO_SIZE - 1);
		fifo->count--;
	}
	return fifo->buffer[fifo->r_index];
}

static inline void uartfifo_push(emulate_uartfifo *fifo, uint8_t ch) {
	if (fifo->count < UARTFIFO_SIZE) {
		fifo->w_index = (fifo->w_index + 1) & (UARTFIFO_SIZE - 1);
		fifo->buffer[fifo->w_index] = ch;
		fifo->count++;
	}
}

#endif
