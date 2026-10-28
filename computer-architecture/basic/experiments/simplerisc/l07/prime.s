	mov r2, 2
.loop:
	mod r3, r1, r2   @ divide number by r2
	cmp r3, 0
	beq .notprime
	add r2, r2, 1
	cmp r1, r2
	bgt .loop
	mov r0, 1        @ prime
	b .exit
.notprime:
	mov r0, 0
.exit:
	nop
