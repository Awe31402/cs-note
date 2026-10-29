# 課本 Example 111（複製）＋ 112（加總）＋ 114（la 與 .word）
	la a1, src
	la a2, dst
	addi t1, zero, 0
	addi t2, zero, 10
.copy:
	lw t0, 0(a1)
	sw t0, 0(a2)
	addi a1, a1, 4
	addi a2, a2, 4
	addi t1, t1, 1
	bne t1, t2, .copy
	la a1, dst              # 加總複製出來的那份
	addi t0, zero, 0
	addi a0, zero, 0
	addi t1, zero, 10
.sum:
	lw t2, 0(a1)
	add a0, a0, t2
	addi a1, a1, 4
	addi t0, t0, 1
	bne t0, t1, .sum
	la t1, val
	lw s0, 0(t1)            # s0 = 17
	ecall
val:	.word 17
src:	.word 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
dst:	.word 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
