#include <asm/assembler.h>

.global	modulo_u64_u64
.global	divide_u64_u64

/**
 * Compute the remainder of the division of [r1:r0] by [r3:r2].
 */
modulo_u64_u64:
	push	{r4-r10}
	mov	r10, #1
	b	2f

/**
 * Divide [r1:r0] by [r3:r2].
 */
divide_u64_u64:
	push	{r4-r10}
	mov	r10, #0

2:
	/* compute bit width of [r3:r2] in [r4] */
	tst	r3, r3
	clzeq	r4, r2
	rsbeq	r4, r4, #32
	clzne	r4, r3
	rsbne	r4, r4, #64

	tst	r4, r4
	beq	int_infty

	/* compute bit width of [r1:r0] in [r5] */
	tst	r1, r1
	clzeq	r5, r0
	rsbeq	r5, r5, #32
	clzne	r5, r1
	rsbne	r5, r5, #64

	// initialize result to 0
	mov	r8, #0
	mov	r9, #0

	cmp	r4, r5
	bgt	div_result		// r4 > r5: division is 0

	// for LSHIFT from [r5-r4 : 0]
	// set [r5] := [r5-r4]
	sub	r5, r5, r4
	// set [r4] := 32 - [r5]
	rsb	r4, r5, #32

	// initialize [r7:r6] = [r3:r2] << [r5]
	lsl	r6, r3, r5
	lsr	r7, r2, r4
	orr	r7, r7, r6
	lsl	r6, r2, r5

divloop:
	// shift result [r9:r8] left by 1
	tst	r8, #0x80000000
	lsl	r9, r9, #1
	lsl	r8, r8, #1
	orrne	r9, r9, #1

	// cmp [r1:r0] > [r7:r6]
	cmp	r1, r7
	cmpeq	r0, r6
	blt	1f

	// if true:
	//   [r1:r0] -= [r7:r6]
	sub	r0, r0, r6
	sbc	r1, r1, r7

	//   increase result
	orr	r8, r8, #1

1:
	// shift [r7:r6] right by 1
	lsrs	r7, r7, #1
	rrx	r6, r6

	subs	r5, r5, #1
	bge	divloop

	// division result is [r9:r8], modulo in [r1:r0]

div_result:
	cmp	r10, #1
	movne	r0, r8
	movne	r1, r9

	pop	{r4-r10}
	bx	lr

int_infty:
	cmp	r10, #1
	moveq	r8, #0
	mvnne	r8, #1
	mov	r9, r8

	pop	{r4-r10}
	bx	lr
ASM_FUNCTION_END divide_u64_u64
ASM_FUNCTION_END modulo_u64_u64
