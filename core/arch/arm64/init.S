#include <asm/assembler.h>
#include <asm/offsets.h>
#include <arch/asm/sysregs.h>
#include <plat/platform_init.h>
#include <asm/memarea.h>

.global	__boot

.pushsection	.text.startup, "ax"

__boot:
	platform_leave_sec

__boot_nonsecure:
	msr	DAIFSet, #0xf
	ic	IALLU
	tlbi	ALLE2

	adr	x3, sysregs
	adr	x24, __boot
	ldp	x8, x9, [x3, #0]
	ldp	x10, x11, [x3, #0x10]
	ldp	x12, x13, [x3, #0x20]
	ldp	x14, x15, [x3, #0x30]
	ldp	x16, x17, [x3, #0x40]
	ldp	x18, x19, [x3, #0x50]
	ldp	x20, x21, [x3, #0x60]
	ldp	x22, x23, [x3], #0x70

	msr	HCR_EL2, x10
	msr	MAIR_EL2, x12
	msr	VBAR_EL2, x13
	msr	TCR_EL2, x9
	msr	VTCR_EL2, x15

	add	x24, x24, x8
	ldr	x25, [x24, #offset(specification, boot_pagetable_address)]
	msr	TTBR0_EL2, x25
	dsb	sy
	isb

	msr	SCTLR_EL2, x11
	isb
	tlbi	ALLE2
	isb

	br	x14

.align	4
sysregs:
	// Delta [__boot, _specification]
	.quad	_specification - __boot
	// TCR_EL2
	.quad	TCR_EL2_FIXEDBITS | TCR_EL2_PS_1T | TCR_EL2_TG_4K | TCR_EL2_SH_OS | TCR_EL2_OC_WBWA | TCR_EL2_IC_WBWA | TCR_EL2_SZ(24)
	// HCR_EL2
	.quad	HCR_EL2_RW | HCR_EL2_TSC | HCR_EL2_TWE | HCR_EL2_TWI | HCR_EL2_AMO | HCR_EL2_IMO | HCR_EL2_FMO | HCR_EL2_VM
	// SCTLR_EL2
	.quad	SCTLR_EL2_FIXEDBITS | SCTLR_EL2_WXN | SCTLR_EL2_I | SCTLR_EL2_C | SCTLR_EL2_M
	// MAIR_EL2
	.quad	MAIR_EL2_NORMAL_NTWBWA(0) | MAIR_EL2_DEVICE(1)
	// VBAR_EL2
	.quad	phidias_vbar
	// Next Boot Step: running with MMU on
	.quad	__boot_paged
	// VTCR_EL2
	.quad	VTCR_EL2_FIXEDBITS | TCR_EL2_PS_1T | TCR_EL2_TG_4K | TCR_EL2_SH_OS | TCR_EL2_OC_WBWA | TCR_EL2_IC_WBWA | VTCR_EL2_SL0(1) | TCR_EL2_SZ(24)
ASM_FUNCTION_END __boot

__boot_paged:
	ldr	x24, =_specification

	mrs	x15, MPIDR_EL1
	plat_mpidr_to_cpuno	x15 x16
	ldr	x25, [x24, #offset(specification, cpus)]

	mov	x4, x15
	tst	x4, x4
	b.eq	2f
1:
	add	x25, x25, #sizeof(specification_cpu)
	subs	x4, x4, #1
	b.ne	1b
2:
	ldr	x26, [x25, #offset(specification_cpu, pagetable_address)]
	msr	TTBR0_EL2, x26
	isb
	tlbi	ALLE2

	ldr	x8, [x25, #offset(specification_cpu, memareas)]
	add	x8, x8, #MEMAREA_STACK * sizeof(memarea)
	ldr	x9, [x8, #offset(memarea, vaddr)]
	ldr	x10, [x8, #offset(memarea, size)]
	add	x9, x9, x10
	mov	sp, x9

	mov	x0, x15
	bl	prepare_areas

	ldr	x1, =cpu_number
	ldr	x2, =top_of_stack
	str	w15, [x1]
	str	x9, [x2]

	ldr	x3, =main
	blr	x3

8:
	wfi
	b	8b
	.ltorg
ASM_FUNCTION_END __boot_paged

.popsection
