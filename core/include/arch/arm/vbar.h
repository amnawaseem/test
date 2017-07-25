#ifndef	__ASSEMBLER__

extern uint32_t	phidias_vbar[8];

#else

.extern	phidias_vbar

#endif

#define	ENTRYFRAME_OFFSET_LR	0x0
#define	ENTRYFRAME_OFFSET_SPSR	0x4
