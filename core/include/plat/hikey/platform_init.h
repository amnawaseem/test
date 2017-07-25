#include <arch/el3_support.h>

#if defined(CONFIG_TWEAK_HIKEY_BOOT_AS_BL1)
	.macro	platform_leave_sec
	nop

	movz	x0, #0xf680, lsl #16
	mov	x1, #5
	set_gicv2_group1 x0, w1

	mov	x1, #0x531
	msr	SCR_EL3, x1

	adr	x2, 1f
	msr	ELR_EL3, x2

	mov	x3, #0x3c9
	msr	SPSR_EL3, x3

	isb

	eret

1:
	.endm
#else
	.macro	platform_leave_sec

	mrs	x1, MPIDR_EL1
	bic	x1, x1, #0xff000000
	tst	x1, x1
	b.ne	1f

	/* if this is the boot processor, execute PSCI CPU_ON */
	movz	x0, #0x0003
	movk	x0, #0xc400, lsl #16
	adr	x2, 1f
	mov	x3, #0
	mov	x7, #0
	smc	#0

	/* disable USER LED0 (GPIO4_0), the green is just too damn bright */
	movz	x0, #0x003c
	movk	x0, #0xf702, lsl #16
	str	wzr, [x0]

1:
	.endm
#endif

	.macro	plat_mpidr_to_cpuno	mpidr_reg scratch
	lsr	\scratch, \mpidr_reg, #6
	and	\scratch, \scratch, #0xc
	ubfx	\mpidr_reg, \mpidr_reg, #0, #2
	add	\mpidr_reg, \mpidr_reg, \scratch
	.endm
