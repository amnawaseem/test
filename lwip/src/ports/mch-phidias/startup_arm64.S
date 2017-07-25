.extern	__lwip_phidias_bss_start
.extern	__lwip_phidias_bss_end

.global	_start

.pushsection .text.startup, "ax", @progbits

_start:
	ldr	x0, =__lwip_phidias_bss_start
	ldr	x1, =__lwip_phidias_bss_end
	ldr	x2, =__lwip_phidias_code_start
1:
	str	xzr, [x0], #8
	cmp	x0, x1
	b.ne	1b
	mov	sp, x2
	b	main

.popsection
