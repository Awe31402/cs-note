# 課本 Example 113：遞迴的階乘（.main 搬到最前面，從位址 0 開始跑）
.main:
	addi a1, zero, 5
	jal ra, .fact
	ecall
.fact:
	addi t1, zero, 1
	bge t1, a1, .ltone
	add s0, a1, zero
	addi a1, a1, -1
	addi sp, sp, -8
	sw ra, 0(sp)
	sw s0, 4(sp)
	jal ra, .fact
	lw s0, 4(sp)
	lw ra, 0(sp)
	addi sp, sp, 8
	mul a0, a0, s0
	jalr zero, 0(ra)
.ltone:
	addi a0, zero, 1
	jalr zero, 0(ra)
