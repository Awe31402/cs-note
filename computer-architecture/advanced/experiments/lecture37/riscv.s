	.text
	.attribute	4, 16
	.attribute	5, "rv64i2p0_m2p0_a2p0_f2p0_d2p0_c2p0"
	.file	"atomics.c"
	.globl	xchg                            # -- Begin function xchg
	.p2align	1
	.type	xchg,@function
xchg:                                   # @xchg
# %bb.0:
	amoswap.d.aqrl	a0, a1, (a0)
	ret
.Lfunc_end0:
	.size	xchg, .Lfunc_end0-xchg
                                        # -- End function
	.globl	tas                             # -- Begin function tas
	.p2align	1
	.type	tas,@function
tas:                                    # @tas
# %bb.0:
	andi	a1, a0, -4
	slliw	a0, a0, 3
	li	a2, 255
	sllw	a2, a2, a0
	li	a3, 1
	sllw	a3, a3, a0
	slli	a3, a3, 32
	srli	a3, a3, 32
.LBB1_1:                                # =>This Inner Loop Header: Depth=1
	lr.w.aqrl	a4, (a1)
	mv	a5, a3
	xor	a5, a5, a4
	and	a5, a5, a2
	xor	a5, a5, a4
	sc.w.aqrl	a5, a5, (a1)
	bnez	a5, .LBB1_1
# %bb.2:
	srlw	a0, a4, a0
	andi	a0, a0, 255
	snez	a0, a0
	ret
.Lfunc_end1:
	.size	tas, .Lfunc_end1-tas
                                        # -- End function
	.globl	faa                             # -- Begin function faa
	.p2align	1
	.type	faa,@function
faa:                                    # @faa
# %bb.0:
	amoadd.d.aqrl	a0, a1, (a0)
	ret
.Lfunc_end2:
	.size	faa, .Lfunc_end2-faa
                                        # -- End function
	.globl	cas                             # -- Begin function cas
	.p2align	1
	.type	cas,@function
cas:                                    # @cas
# %bb.0:
.LBB3_1:                                # =>This Inner Loop Header: Depth=1
	lr.d.aqrl	a3, (a0)
	bne	a3, a1, .LBB3_3
# %bb.2:                                #   in Loop: Header=BB3_1 Depth=1
	sc.d.aqrl	a4, a2, (a0)
	bnez	a4, .LBB3_1
.LBB3_3:
	xor	a0, a3, a1
	seqz	a0, a0
	ret
.Lfunc_end3:
	.size	cas, .Lfunc_end3-cas
                                        # -- End function
	.globl	unlock_release                  # -- Begin function unlock_release
	.p2align	1
	.type	unlock_release,@function
unlock_release:                         # @unlock_release
# %bb.0:
	fence	rw, w
	sw	zero, 0(a0)
	ret
.Lfunc_end4:
	.size	unlock_release, .Lfunc_end4-unlock_release
                                        # -- End function
	.ident	"Ubuntu clang version 14.0.0-1ubuntu1.1"
	.section	".note.GNU-stack","",@progbits
	.addrsig
