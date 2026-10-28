	addi t0, zero, 4
	addi t1, zero, 5
	add  t2, t0, t1
	addi t2, t2, -19      # 沒有 subi：加 -19
	addi a0, zero, 10     # swap a0, a1
	addi a1, zero, 20
	add  a2, a0, zero
	add  a0, a1, zero
	add  a1, a2, zero
	ecall
