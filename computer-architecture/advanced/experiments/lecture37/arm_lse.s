	.arch armv8.1-a+crc
	.file	"atomics.c"
	.text
	.align	2
	.p2align 4,,11
	.global	xchg
	.type	xchg, %function
xchg:
.LFB0:
	.cfi_startproc
	swpal	x1, x0, [x0]
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
	mov	w1, 1
	swpalb	w1, w1, [x0]
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
	ldaddal	x1, x0, [x0]
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
	mov	x3, x1
	casal	x3, x2, [x0]
	cmp	x3, x1
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
