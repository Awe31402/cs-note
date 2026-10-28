	li   t0, 409932
	li   t1, 409823
	add  t2, t0, t1
	li   t3, 0x12345FFF   # 低 12 位 ≥ 0x800：lui 要多加 1
	li   t4, -5           # 小的數只要一條 addi
	ecall
