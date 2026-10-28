	.text
	la    a1, v
	flw   ft0, 0(a1)          # 2.5
	flw   ft1, 4(a1)          # 3.5
	flw   ft2, 8(a1)          # -2.5
	fcvt.w.s s0, ft0, rne
	fcvt.w.s s1, ft1, rne
	fcvt.w.s s2, ft2, rne
	fcvt.w.s s3, ft0, rtz
	fcvt.w.s s4, ft2, rtz
	fcvt.w.s s5, ft2, rdn
	fcvt.w.s s6, ft2, rup
	fcvt.w.s s7, ft0, rmm
	fcvt.w.s s8, ft2, rmm
	ecall
	.data
v:	.float 2.5, 3.5, -2.5
