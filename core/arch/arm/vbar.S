.global	phidias_vbar

.align	5
phidias_vbar:
	b	.			// reset
	b	arm_vector_undefined
	b	arm_vector_supervisor_call
	b	arm_vector_prefetch_abort
	b	arm_vector_data_abort
	b	.			// unused
	b	arm_vector_interrupt
	b	.			// arm_vector_fast_interrupt
