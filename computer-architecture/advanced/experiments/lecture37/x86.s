	.file	"atomics.c"
	.text
	.p2align 4
	.globl	xchg
	.type	xchg, @function
xchg:
.LFB0:
	.cfi_startproc
	endbr64
	movq	%rsi, %rax
	xchgq	(%rdi), %rax
	ret
	.cfi_endproc
.LFE0:
	.size	xchg, .-xchg
	.p2align 4
	.globl	tas
	.type	tas, @function
tas:
.LFB1:
	.cfi_startproc
	endbr64
	movl	$1, %eax
	xchgb	(%rdi), %al
	movzbl	%al, %eax
	ret
	.cfi_endproc
.LFE1:
	.size	tas, .-tas
	.p2align 4
	.globl	faa
	.type	faa, @function
faa:
.LFB2:
	.cfi_startproc
	endbr64
	movq	%rsi, %rax
	lock xaddq	%rax, (%rdi)
	ret
	.cfi_endproc
.LFE2:
	.size	faa, .-faa
	.p2align 4
	.globl	cas
	.type	cas, @function
cas:
.LFB3:
	.cfi_startproc
	endbr64
	movq	%rsi, %rax
	lock cmpxchgq	%rdx, (%rdi)
	sete	%al
	movzbl	%al, %eax
	ret
	.cfi_endproc
.LFE3:
	.size	cas, .-cas
	.p2align 4
	.globl	unlock_release
	.type	unlock_release, @function
unlock_release:
.LFB4:
	.cfi_startproc
	endbr64
	movl	$0, (%rdi)
	ret
	.cfi_endproc
.LFE4:
	.size	unlock_release, .-unlock_release
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
