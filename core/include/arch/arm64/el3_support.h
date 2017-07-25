#ifdef __ASSEMBLER__

# include <asm/gic.h>

.extern	smc_handler

	// Setup and enable the GIC from secure mode.
	// Clobbers: {x20, x21, x22, x23}
	.macro	set_gicv2_group1	gic_base nregs
	mov	w20, \nregs
	mov	w21, #0xffffffff
	add	x22, \gic_base, #GICV2_AREAOFFSET_DIST
	add	x23, x22, #GIC_DIST_GROUP_BASE
1:
	str	w21, [x23], #4
	subs	w20, w20, #1
	bne	1b

	mov	w20, \nregs
	movz	w21, #0x9090
	movk	w21, #0x9090, lsl #16
	add	x23, x22, #GIC_DIST_PRIORITY_BASE
1:
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	subs	w20, w20, #1
	bne	1b

	mov	w20, \nregs
	movz	w21, #0x0101
	movk	w21, #0x0101, lsl #16
	add	x23, x22, #GIC_DIST_TARGETS_BASE
1:
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	str	w21, [x23], #4
	subs	w20, w20, #1
	bne	1b

	mov	w21, #3		// DIST_EN0, DIST_EN1
	str	w21, [x22, #GIC_DIST_CTRL]

	add	x22, \gic_base, #GICV2_AREAOFFSET_CPU
	mov	w21, #0xff		// minimum PMR
	str	w21, [x22, #GIC_CPU_PMR]
	mov	w21, #0x03		// CPU_EN0, CPU_EN1
	str	w21, [x22, #GIC_CPU_CTRL]
	dsb	nsh
	.endm

	.macro	enable_nonsecure_coprocs
	msr	CPTR_EL3, xzr
	.endm

#endif
