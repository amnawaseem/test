.global	smc_handler

smc_handler:
	cmp	r0, #0
	bne	1f

	// trigger a secure SGI to all other CPUs
	ldr	r0, =0x1e001f00		// VEx-A9 MPCore: GIC Distributor SGIR
	ldr	r1, =0x01000005		// ALL_EXCEPT_SELF | PHIDIAS_SGI_VECTOR
	str	r1, [r0]
	dsb	sy

	movs	pc, lr
	.ltorg

1:
	b	.
	movs	pc, lr
