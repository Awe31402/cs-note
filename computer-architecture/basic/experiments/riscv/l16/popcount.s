# 課本 Example 106：a1 裡有幾個 1
	addi t0, zero, 0
	addi t1, zero, 32
	addi t2, zero, 1
	addi a0, zero, 0
.loop:
	andi t3, a1, 1
	srli a1, a1, 1
	beq t3, t2, .inc
.lret:
	addi t0, t0, 1
	beq t1, t0, .end
	jal zero, .loop
.inc:
	addi a0, a0, 1
	jal zero, .lret
.end:
	ecall
