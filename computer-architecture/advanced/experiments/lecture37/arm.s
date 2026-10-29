	.arch armv8-a
	.file	"atomics.c"
	.text
	.align	2
	.p2align 4,,11
	.global	xchg
	.type	xchg, %function
xchg:
.LFB0:
	.cfi_startproc
	mov	x2, x0
.L3:
	ldaxr	x0, [x2]
	stlxr	w3, x1, [x2]
	cbnz	w3, .L3
	ret
	.cfi_endproc
.LFE0:
	.size	xchg, .-xchg
	.align	2
	.p2align 4,,11
	.global	tas
	.type	tas, %function
tas:
.LFB1:
	.cfi_startproc
	mov	w2, 1
.L5:
	ldaxrb	w1, [x0]
	stlxrb	w3, w2, [x0]
	cbnz	w3, .L5
	and	w0, w1, 255
	ret
	.cfi_endproc
.LFE1:
	.size	tas, .-tas
	.align	2
	.p2align 4,,11
	.global	faa
	.type	faa, %function
faa:
.LFB2:
	.cfi_startproc
	mov	x2, x0
.L7:
	ldaxr	x0, [x2]
	add	x3, x0, x1
	stlxr	w4, x3, [x2]
	cbnz	w4, .L7
	ret
	.cfi_endproc
.LFE2:
	.size	faa, .-faa
	.align	2
	.p2align 4,,11
	.global	cas
	.type	cas, %function
cas:
.LFB3:
	.cfi_startproc
.L9:
	ldaxr	x3, [x0]
	cmp	x3, x1
	bne	.L10
	stlxr	w4, x2, [x0]
	cbnz	w4, .L9
.L10:
	cset	w0, eq
	ret
	.cfi_endproc
.LFE3:
	.size	cas, .-cas
	.align	2
	.p2align 4,,11
	.global	unlock_release
	.type	unlock_release, %function
unlock_release:
.LFB4:
	.cfi_startproc
	stlr	wzr, [x0]
	ret
	.cfi_endproc
.LFE4:
	.size	unlock_release, .-unlock_release
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
