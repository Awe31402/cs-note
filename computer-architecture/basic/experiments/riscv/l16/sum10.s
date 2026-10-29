# 課本 Example 104：1 加到 10
	addi t0, zero, 1
	addi s0, zero, 0
	addi t1, zero, 10
.loop:
	add s0, s0, t0
	addi t0, t0, 1
	bge t1, t0, .loop
	ecall
