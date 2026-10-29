# 課本 Example 102：<t1,t0> + <t3,t2> = <t5,t4>，用 sltu 算進位
	addi t2, zero, -1       # <t3,t2> = 0x00000002_FFFFFFFF
	addi t3, zero, 2
	addi t0, zero, 1        # <t1,t0> = 0x00000000_00000001
	addi t1, zero, 0
	add t4, t0, t2          # 低 32 位元
	add t5, t1, t3          # 高 32 位元
	sltu t6, t4, t0         # 結果比加數小（unsigned）→ 有進位
	add t5, t5, t6
	ecall
