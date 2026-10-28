	li   t1, 1
	li   t2, 12
	mul  t3, t2, t2
	mul  t3, t3, t2       # 12^3
	add  t4, t3, t1       # 12^3 + 1
	li   a0, 4
	ori  a1, a0, 1        # 4 | 1
	li   a2, 3
	li   a3, 7
	slli a4, a3, 2        # a3 × 4
	add  a5, a2, a4       # a2 + a3 × 4
	ecall
