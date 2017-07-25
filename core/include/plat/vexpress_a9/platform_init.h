#include <arch/sec_support.h>

#define	SCU_CONTROL	0x000
#define	SCU_SAC		0x050
#define	SCU_SNSAC	0x054

#define	SCU_CONTROL_ENABLE	0x01
#define	SCU_SAC_ALL		0x0f
#define	SCU_SNSAC_ALL		0xfff

	.macro	platform_leave_sec
	ldr	r0, =0x1e001000
	ldr	r1, =0x1e000100
	mov	r2, #3
	set_gic_group1	r0, r1, r2

	install_secure_vbar

	enable_nonsecure_coprocs

	ldr	r0, =0x1e000000
	mov	r1, #SCU_CONTROL_ENABLE
	str	r1, [r0, #SCU_CONTROL]
	mov	r1, #SCU_SAC_ALL
	str	r1, [r0, #SCU_SAC]
	movw	r1, #SCU_SNSAC_ALL
	str	r1, [r0, #SCU_SNSAC]

	ldr	r1, =(PSR_MODE_SUPERVISOR | PSR_IRQMASK | PSR_FIQMASK)
	secure_exit	r1
	.endm

	.macro	plat_mpidr_to_cpuno	mpidr_reg scratch
	and	\mpidr_reg, \mpidr_reg, #0xff
	.endm
