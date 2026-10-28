	.arm
	.syntax unified
	mov	r1, r2
	mov	r1, #3
	mvn	r1, r2
	mvn	r1, #3
	add	r1, r2, #3
	rsb	r2, r1, #3
	bic	r1, r2, r3
	eor	r1, r2, r3
	orr	r1, r2, r3
	mul	r4, r0, r0
	mla	r3, r4, r0, r1
	smull	r0, r1, r2, r3
	umull	r0, r1, r2, r3
	mov	r1, r2, asr #2
	add	r1, r2, r3, lsl #2
	mov	r1, r2, ror #1
	mov	r1, r2, rrx
	cmp	r1, r2
	cmn	r1, #1
	tst	r1, #1
	teq	r1, r2
	adds	r5, r1, r3
	adc	r6, r2, r4
	subs	r5, r1, r3
	sbc	r6, r2, r4
