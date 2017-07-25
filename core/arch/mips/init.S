#include <asm/assembler.h>
#include <arch/asm/cp0_regs.h>

.global	__boot

.pushsection    .text.startup, "ax"

__boot:
//	jal 	prepare_areas
	sw 	$sp, top_of_stack
	la	$gp, _gp

	/* set EXL in status - needed for first upcall, afterwards
	 * always already set */
	mfc0	$t1, c0_status
	ori	$t1, $t1, 0x2
	mtc0	$t1, c0_status

	/* extract number of TLB entries from CP0_Config1 */
	mfc0	$t0, c0_config1
	srl	$t0, $t0, 25
	andi	$t0, $t0, 0x3F
	sw	$t0, max_tlb_index

	/* set K0 in Config to 2 meaning Kseg0 is uncached */
	mfc0	$t0, c0_config0
	ori	$t0, 7
	xori	$t0, 5
	mtc0	$t0, c0_config0

	/* set LED pins to out, turn all on */
	lui	$t0, 0xbf86
	li	$t1, 0x700
	sw	$t1, 0x114($t0)
	sw	$t1, 0x138($t0)

	/* supply valid $sp for all shadow sets */
	mfc0	$t1, c0_srsctl
	srl	$t0, $t1, 26
1:
	ins	$t1, $t0, 6, 4
	mtc0	$t1, c0_srsctl
	ehb
	wrpgpr	$sp, $sp
	wrpgpr	$gp, $gp
	addiu	$t0, $t0,-1
	bgez	$t0, 1b
	nop

	/* set Shadow Set 1 as set for EXC */
	li	$t0, 1
	mfc0	$t1, c0_srsctl
	ins	$t1, $t0, 12, 4
	mtc0	$t1, c0_srsctl

	j	main
	nop
ASM_FUNCTION_END __boot

.popsection
