	sub r1, r2, 3
	st r8, 20[r2]
	add r1, r2, r3
	movh r0, 0xFB12
	addu r0, r0, 0xCDEF
	cmp r2, 1
	nop
	ret
