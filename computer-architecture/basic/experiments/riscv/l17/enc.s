	add  t2, t0, t1
	sub  t2, t0, t1
	xor  t2, t0, t1
	addi t1, t0, -19
	lw   a0, 8(sp)
	jalr zero, 0(ra)
	sw   a0, 8(sp)
	lui  t0, 100
.L1:
	beq  t0, t1, .L2
	nop
.L2:
	jal  ra, .L1
	fadd.s  ft0, ft1, ft2
	fmadd.s fa0, fs1, fs2, ft4
	flw  ft0, 4(a1)
