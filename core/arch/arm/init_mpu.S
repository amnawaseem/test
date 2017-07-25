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
	ldmia	r3!, {r4-r7}

	mcr	VBAR(r5)
	mcr	CPACR(r6)
	vmsr	fpexc, r7

	b	__boot_paged
cp15consts:
	// SCTLR: {FI, DZ, !BR, I, Z, C, M}
	.long	0x00ed185d
	// VBAR
	.long	phidias_vbar
	// CPACR: all coprocessors available for PL0
	.long	0x0fffffff
	// FPEXC: activate VFP
	.long	FPEXC_EN

ASM_FUNCTION_END __boot

__boot_paged:
	ldr	sp, =_specification

	mrc	MPIDR(r5)
        plat_mpidr_to_cpuno     r5 r6
	ldr	sp, [sp, #offset(specification, cpus)]
	mov	r7, r5
2:
	tst	r7, r7
	addne	sp, sp, #sizeof(specification_cpu)
	subne	r7, r7, #1
	bne	2b
	ldr	r6, [sp, #offset(specification_cpu, pagetable_address)]

1:
	ldmia	r6!, {r0-r3}
	tst	r0, r0
	beq	3f
	mcr	RGNR(r0)
	mcr	DRBAR(r1)
	mcr	DRSR(r2)
	mcr	DRACR(r3)
	b	1b

3:
	ldr	r8, [sp, #offset(specification_cpu, memareas)]
	add	r8, r8, #MEMAREA_STACK * sizeof(memarea)
	ldr	sp, [r8, #offset(memarea, vaddr)]
	ldr	r6, [r8, #offset(memarea, size)]
	add	sp, sp, r6
	sub	sp, sp, #4

	mov	r0, r5
	bl	prepare_areas

	mcr	SCTLR(r4)
	isb

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
