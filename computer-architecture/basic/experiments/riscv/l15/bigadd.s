	lui  t0, 100          # t0 = 100 << 12 = 409600
	addi t0, t0, 332      # t0 = 409932
	lui  t1, 100
	addi t1, t1, 223      # t1 = 409823
	add  t2, t0, t1       # t2 = 819755
	ecall
