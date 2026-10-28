	.text
	la    a1, pi
	flw   fs1, 0(a1)          # fs1 = pi
	la    a2, e
	flw   fs2, 0(a2)          # fs2 = e
	fadd.s ft1, fs1, fs2      # pi + e
	fmul.s ft2, fs1, fs2      # pi * e
	fadd.s ft3, ft1, ft2      # pi + e + pi * e
	fsqrt.s fa0, ft3          # Example 116
	addi  t1, zero, 4
	fcvt.s.w ft4, t1          # 4 -> 4.0
	fmadd.s fa1, fs1, fs2, ft4   # Example 117：pi*e + 4
	fcvt.w.s a0, fa1          # 預設捨入（DYN → fcsr.frm = RNE）
	fcvt.w.s a3, fa1, rtz     # 往 0 捨入 = C 的 (int)
	add   a4, zero, zero      # Example 118
	flt.s t0, fs2, fs1        # e < pi ?
	beq   t0, zero, .end
	addi  a4, zero, 17
.end:
	ecall
	.data
pi:	.float 3.14
e:	.float 2.72
