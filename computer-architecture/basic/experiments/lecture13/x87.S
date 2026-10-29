# x87.S：第 13 支的字串指令和 x87 浮點指令（Intel 語法，x86-64 System V 呼叫慣例）
	.intel_syntax noprefix
	.text
	.globl ex81, copy_rep, copy_back, fist_of, fisttp_of, amean, gmean, sin_check

ex81:	# 課本 Example 81：movsd 之後讀回 [esp+8]
	sub rsp, 24
	mov dword ptr [rsp+4], 192
	lea rsi, [rsp+4]
	lea rdi, [rsp+8]
	movsd                        # 沒有運算元：字串的 movs dword，不是 SSE 的 movsd
	mov eax, [rsp+8]
	add rsp, 24
	ret

copy_rep:	# copy_rep(dst, src, n)：課本 Example 84；回傳之後的 ecx
	mov rcx, rdx
	cld
	rep movsd
	mov rax, rcx
	ret

copy_back:	# copy_back(dst, src, n)：DF = 1，從最後一個往前搬
	lea rsi, [rsi + rdx*4 - 4]
	lea rdi, [rdi + rdx*4 - 4]
	mov rcx, rdx
	std
	rep movsd
	cld                          # ABI 規定回去時 DF 要是 0
	ret

fist_of:	# fist_of(double*)：fld 再 fistp，回傳整數
	fld qword ptr [rdi]
	fistp dword ptr [rsp-8]
	mov eax, [rsp-8]
	ret

fisttp_of:	# fisttp（SSE3 加的）：一定截斷
	fld qword ptr [rdi]
	fisttp dword ptr [rsp-8]
	mov eax, [rsp-8]
	ret

amean:	# amean(a, b, double *out)：影片的算術平均
	mov [rsp-8], edi
	mov [rsp-16], esi
	fild dword ptr [rsp-8]
	fild dword ptr [rsp-16]
	fadd st(0), st(1)
	fdiv dword ptr [rip + two]
	fstp qword ptr [rdx]
	fstp st(0)                  # 把剩下的那個也清掉
	ret

gmean:	# gmean(a, b, double *out)：幾何平均 = sqrt(a*b)
	mov [rsp-8], edi
	mov [rsp-16], esi
	fild dword ptr [rsp-8]
	fild dword ptr [rsp-16]
	fmul st(0), st(1)
	fsqrt
	fstp qword ptr [rdx]
	fstp st(0)
	ret

sin_check:	# sin_check(float theta, float threshold, int use_jg)：課本 Example 89
	movss [rsp-4], xmm0
	movss [rsp-8], xmm1
	fld dword ptr [rsp-4]
	fadd st(0), st(0)
	fsin
	fstp dword ptr [rsp-12]      # [rsp-12] = sin(2θ)
	fld dword ptr [rsp-4]
	fst st(1)
	fsin
	fxch
	fcos
	fmul st(0), st(1)
	fadd st(0), st(0)            # st0 = 2 sinθ cosθ
	fld dword ptr [rsp-12]
	fsub st(0), st(1)
	fabs
	fld dword ptr [rsp-8]        # st0 = threshold
	fcomi st(0), st(1)
	seta al                      # 對的：unsigned 的 above（CF = 0 且 ZF = 0）
	setg cl                      # 錯的：signed 的 greater（ZF = 0 且 SF = OF）
	finit                        # 清空 FP stack（不影響 EFLAGS）
	test edi, edi                # 第三個參數 use_jg：兩個 float 用 xmm0／xmm1，int 用 edi
	cmovnz eax, ecx
	movzx eax, al
	ret

	.section .rodata
two:	.float 2.0
	.section .note.GNU-stack,"",@progbits
