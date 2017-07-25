/**
 * The Raspberry Pi 3 bootup for AArch64 is the stupidest thing ever.
 * It requires setting
 *   arm_control=0x200
 *   kernel_old=1
 * which boots the kernel at address 0x0 (matching RVBAR_EL3).
 * However the bootloader also builds ATAGS for us which are placed at
 * 0x100 (yay, pure genius!). Disabling this crap with
 *   disable_commandline_tags=1
 * seems to break boot completely, so that is no option either.
 * So blow ourselves up with a lot of padding to jump across the ATAGS
 * that are thrown on top of us...
 */

#if defined(CONFIG_TWEAK_RPI3_NO_ARMSTUB)
	.macro	platform_leave_sec
	b	99f

	.rep	511
	nop
	.endr
99:
	mov	x1, #0x531
	msr	SCR_EL3, x1
	dsb	sy
	isb

	mov	x1, #0x40
	msr	CPUECTLR_EL1, x1
/*			// If in desperation: switch the A53 back to pedestrian speed...
	isb
	mrs	x1, CPUACTLR_EL1
	orr	x1, x1, #0x60000000		// disable dual-issue
	bic	x1, x1, #0x00180000		// data prefetch streams -> 1
	bic	x1, x1, #0x0000e000		// L1 data prefetch control -> OFF
	msr	CPUACTLR_EL1, x1
	isb
*/

	movz	x1, #0x0124, lsl #16
	movk	x1, #0xf800
	msr	CNTFRQ_EL0, x1

	adr	x7, 1f
	msr	ELR_EL3, x7

	mov	x6, #0x3c9
	msr	SPSR_EL3, x6
	isb

	mrs	x0, MPIDR_EL1
	ands	x0, x0, #0xff
	b.ne	98f
	eret

98:
	adr	x4, __boot
	ldr	w3, 96f
	add	x3, x3, x4		// x3: core_physical_address(multicore_boot_aux)

	sevl
97:
	wfe
	ldr	w4, [x3]
	cmp	w4, #2
	b.ne	97b
	eret
96:
	.long	multicore_boot_aux - __boot

1:
	.endm
#else
	.macro	platform_leave_sec
	.endm
#endif

	.macro  plat_mpidr_to_cpuno	mpidr_reg scratch
	and	\mpidr_reg, \mpidr_reg, #0xff
	.endm
