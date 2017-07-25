#ifndef _PHIDIAS_XCORE_H_
#define _PHIDIAS_XCORE_H_

extern void capability_invoke(uint32_t cap_number);

extern uint32_t xcore_ipi_interrupt(void);
extern void xcore_virtual_ipi(const struct vm_ *, uint32_t, uint32_t, uint32_t);

extern void xcore_setup(void);

#define		XCORE_BUFFER_SIZE	( (0x1000 - sizeof(uint32_t) - sizeof(uint32_t)) / sizeof(wordsize_t) )

struct xcore_buffer_ {
	uint32_t	reverse_read_offset;
	uint32_t	write_offset;

	wordsize_t	buffer[XCORE_BUFFER_SIZE];
};

typedef struct xcore_buffer_	xcore_buffer;

#define		XCOREMESSAGE_VIPI	1
#define		XCOREMESSAGE_REMOTE_IRQ	2

#endif /* _PHIDIAS_XCORE_H_ */
