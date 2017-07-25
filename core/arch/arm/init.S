#include <asm/assembler.h>
#include <asm/offsets.h>
#include <arch/asm/cp15_regs.h>
#include <arch/asm/cpu_ops.h>
#include <plat/platform_init.h>
#include <asm/memarea.h>

.global	__boot

.pushsection    .text.startup, "ax"

__boot:
	platform_leave_sec

__boot_nonsecure:
	cpsid	afi

	adr	r3, cp15consts
	adr	sp, __boot
	ldmia	r3!, {r4-r12}
	add	sp, sp, r12
	ldr	r12, [sp, #offset(specification, boot_pagetable_address)]
	orr	r12, r12, #0x4a			// TTBR0: RGN/IRGN Normal WB/WA, Shareable

	mcr	DACR(r4)
	mcr	NMRR(r6)
	mcr	PRRR(r7)
	mcr	TTBCR(r8)
	mcr	TTBR0(r12)
	mcr	VBAR(r9)
	mcr	CONTEXTIDR(r10)

	ldmia	r3!, {r6-r7}
	mcr	CPACR(r6)
	vmsr	fpexc, r7

	mcr	SCTLR(r5)
	isb

	bx	r11
cp15consts:
	// DACR: Client for domain #0
	.long	DACR_CLIENT(0)
	// SCTLR: {AFE, TRE, I, C, Z, M}
	.long	0x30c5185d
	// NMRR: OR[0] WB/WA, IR[0] WB/WA, OR[1-7] NC, IR[1-7] NC
	.long	0x00010001
	// PRRR: {NS1, DS1}, TR[0] NormalMem, TR[1-7] StronglyOrdered
	.long	0x000a0002
	// TTBCR: {PD1}, N 0
	.long	0x00000020
	// VBAR
	.long	phidias_vbar
	// CONTEXTIDR
	.long	1
	// Next Boot Step: running with MMU on
	.long	__boot_paged
	// Delta [__boot, _specification]
	.long	_specification - __boot

	// CPACR: all coprocessors available for PL0
	.long	0x0fffffff
	// FPEXC: activate VFP
	.long	FPEXC_EN
ASM_FUNCTION_END __boot

__boot_paged:
	ldr	sp, =_specification

	mrc	MPIDR(r5)
	and	r5, r5, #0xff
	ldr	sp, [sp, #offset(specification, cpus)]
	mov	r7, r5
2:
	tst	r7, r7
	addne	sp, sp, #sizeof(specification_cpu)
	subne	r7, r7, #1
	bne	2b
	ldr	r6, [sp, #offset(specification_cpu, pagetable_address)]

	mcr	TTBR0(r6)
	dsb	nsh
	isb

	mcr	TLBIALL

	ldr	r4, [sp, #offset(specification_cpu, memareas)]
	add	r4, r4, #MEMAREA_STACK * sizeof(memarea)
	ldr	sp, [r4, #offset(memarea, vaddr)]
	ldr	r6, [r4, #offset(memarea, size)]
	add	sp, sp, r6
	sub	sp, sp, #4

	mov	r0, r5
	bl	prepare_areas

	ldr	r4, =cpu_number
	ldr	r6, =top_of_stack
	str	r5, [r4]
	str	sp, [r6]

	ldrex	r1, [sp]
	add	r1, r1, #1
	strex	r2, r1, [sp]

	bl	main

8:
	wfi
	b	8b

	.ltorg
ASM_FUNCTION_END __boot_paged

.popsection
