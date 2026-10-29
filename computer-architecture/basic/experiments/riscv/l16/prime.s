# 課本 Example 105：a1 是不是質數
	addi t0, zero, 2
.loop:
	rem t1, a1, t0
	beq t1, zero, .notPrime
	addi t0, t0, 1
	bne t0, a1, .loop
	addi a0, zero, 1
	jal x0, .end
.notPrime:
	addi a0, zero, 0
.end:
	ecall
