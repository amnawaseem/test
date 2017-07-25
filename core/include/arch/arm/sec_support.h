#ifdef __ASSEMBLER__

# include <asm/gic.h>

.extern	smc_handler

	// Setup and enable the GIC from secure mode.
	// Clobbers: {ip, fp, sl}
	.macro	set_gic_group1	dist_base cpu_base nregs
	mov	ip, \nregs
	mov	fp, #0xffffffff
	add	sl, \dist_base, #GIC_DIST_GROUP_BASE
1:
	str	fp, [sl], #4
	subs	ip, ip, #1
	bne	1b

	mov	ip, \nregs
	movw	fp, #0x9090
	orr	fp, fp, fp, lsl #16
	add	sl, \dist_base, #GIC_DIST_PRIORITY_BASE
1:
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	subs	ip, ip, #1
	bne	1b

	mov	ip, \nregs
	movw	fp, #0x0101
	orr	fp, fp, fp, lsl #16
	mov	sl, \dist_base
	add	sl, sl, #GIC_DIST_TARGETS_BASE
1:
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	str	fp, [sl], #4
	subs	ip, ip, #1
	bne	1b

	mov	sl, \dist_base
	mov	fp, #3		// DIST_EN0, DIST_EN1
	str	fp, [sl, #GIC_DIST_CTRL]

	mov	sl, \cpu_base
	mov	fp, #0xff		// minimum PMR
	str	fp, [sl, #GIC_CPU_PMR]
	mov	fp, #0x03		// CPU_EN0, CPU_EN1
	str	fp, [sl, #GIC_CPU_CTRL]
	dsb	nsh
	.endm

	.macro	secure_exit	exit_psr
	adr	ip, 9f
	cps	#PSR_MODE_MONITOR
	mrc	SCR(fp)
	orr	fp, fp, #SCR_NS_BIT
	mcr	SCR(fp)
	mov	fp, \exit_psr
	msr	SPSR_sfxc, fp
	movs	pc, ip
	.ltorg
9:
	.endm

	.macro	enable_nonsecure_coprocs
	movw	fp, #0x3fff
	mcr	NSACR(fp)
	.endm

	.macro	install_secure_vbar
	adr	ip, secure_vbar
	cps	#PSR_MODE_MONITOR
	mcr	MVBAR(ip)
	cps	#PSR_MODE_SUPERVISOR
	b	9f

.align	5
secure_vbar:
	movs	pc, lr
	movs	pc, lr
	b	smc_handler
	movs	pc, lr
	movs	pc, lr
	movs	pc, lr
	movs	pc, lr
	movs	pc, lr

9:
	.endm

#else

extern void smc_call(uint32_t);

#endif
