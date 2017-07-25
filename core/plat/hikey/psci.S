.global	psci_call

psci_call:
	stp	x8, x9, [sp, #-16]
	stp	x10, x11, [sp, #-32]
	stp	x12, x13, [sp, #-48]
	stp	x14, x15, [sp, #-64]!
	mov	x7, #0
	smc	#0
	ldp	x8, x9, [sp, #48]
	ldp	x10, x11, [sp, #32]
	ldp	x12, x13, [sp, #16]
	ldp	x14, x15, [sp], #64
	mov	x0, #0x8000000
1:
	subs	x0, x0, #1
	b.ne	1b
	ret
