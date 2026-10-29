# 課本 Example 103：迴圈算階乘，n 在 a1
	addi a0, zero, 1
	addi t0, zero, 1
.loop:
	mul a0, a0, t0
	addi t0, t0, 1
	bge a1, t0, .loop
	ecall
