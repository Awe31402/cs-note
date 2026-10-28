	.arm
	.syntax unified
power:
	cmp	r1, #0
	moveq	r0, #1
	bxeq	lr
	stmfd	sp!, {r4, lr}
	mov	r4, r0
	sub	r1, r1, #1
	bl	power
	mul	r0, r4, r0
	ldmfd	sp!, {r4, pc}
imm:
	mov	r0, #42
	mov	r0, #0x2a00
	mov	r0, #0xfe000000
	mov	r0, #0xe000000f
	add	r0, r0, #0x2a
mem:
	ldr	r1, [r0]
	ldr	r1, [r0, #4]
	ldr	r1, [r0, #-4]
	ldr	r1, [r0, r2]
	ldr	r3, [r0, r2, lsl #2]
	ldr	r3, [r0], r2, lsl #2
	ldr	r0, [r1, #4]!
	ldr	r3, [r0], #4
	ldrb	r1, [r0, #2]
	str	r1, [r0]
loop:
	addeq	r4, r4, #1
	adds	r4, r4, #1
	mov	r1, r1, lsr #1
	b	loop
