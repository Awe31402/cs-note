# 課本 Example 108 ＋ 109：jal 呼叫、jalr zero, 0(ra) 回來
.main:
	addi s0, zero, 3
	jal ra, .foo
	add s1, s0, a0          # y = x + foo() = 5
	addi a1, zero, 7
	addi a2, zero, 3
	jal ra, .power          # 7^3
	ecall
.foo:
	addi a0, zero, 2
	jalr zero, 0(ra)
.power:
	addi a0, zero, 1
	add t1, zero, a2
	beq t1, zero, .end
.loop:
	mul a0, a0, a1
	addi t1, t1, -1
	bne t1, zero, .loop
.end:
	jalr zero, 0(ra)
