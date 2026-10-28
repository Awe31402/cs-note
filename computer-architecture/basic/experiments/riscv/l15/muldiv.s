	li   t1, 3
	li   t2, -17
	mul  t3, t1, t2       # 3 × -17
	li   s0, -50
	li   s1, 3
	div  s2, s0, s1       # -50 / 3
	rem  s3, s0, s1       # -50 % 3
	li   s4, 4
	li   s5, -3
	div  s6, s4, s5       # 4 / -3
	rem  s7, s4, s5
	li   s8, 0x7FFFFFFF
	mulh  s9, s8, s8      # (2^31-1)^2 的高 32 位
	mul   s10, s8, s8     # 低 32 位
	ecall
