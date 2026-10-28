; 課本 Ch5 的例子，包成 C 可以呼叫的 32 位元函式（cdecl：參數在 [esp+4]，回傳值在 eax）
bits 32
section .text
global ex60, ex67_q, ex67_r, shifts_sal, shifts_sar, shifts_shr, prime, fact74, fact78, cf_after_sub

ex60:                       ; Example 60：push dword [esp]
	push ebp
	push ebx
	mov ebp, esp
	sub esp, 16
	mov eax, 10
	mov [esp], eax
	push dword [esp]
	mov ebx, [esp]
	lea ecx, [ebp-16]       ; 原本的 esp
	sub ecx, esp            ; push 之後 esp 少了幾 byte
	mov eax, ebx
	shl ecx, 16
	or eax, ecx             ; 高 16 位：esp 的變化量，低 16 位：ebx
	mov esp, ebp
	pop ebx
	pop ebp
	ret

ex67_q:                     ; Example 67：-50 / 3
	push ebx
	mov edx, -1
	mov eax, -50
	mov ebx, 3
	idiv ebx
	pop ebx
	ret                     ; 商在 eax
ex67_r:
	push ebx
	mov edx, -1
	mov eax, -50
	mov ebx, 3
	idiv ebx
	mov eax, edx            ; 餘數
	pop ebx
	ret

shifts_sal: mov eax, 0xdeadc0de
	sal eax, 4
	ret
shifts_sar: mov eax, 0xdeadc0de
	sar eax, 4
	ret
shifts_shr: mov eax, 0xdeadc0de
	shr eax, 4
	ret

prime:                      ; Example 72（輸入 [esp+4]）
	push ebx
	mov eax, [esp+8]
	mov ebx, 2
	mov ecx, eax
.loop:
	mov edx, 0
	idiv ebx
	cmp edx, 0
	je .notprime
	inc ebx
	mov eax, ecx
	cmp ebx, eax
	jl .loop
	mov eax, 1
	jmp .exit
.notprime:
	mov eax, 0
.exit:
	pop ebx
	ret

fact74:                     ; Example 74 的外殼：輸入放 eax，結果在 ebx
	push ebx
	mov eax, [esp+8]
	call factorial74
	mov eax, ebx
	pop ebx
	ret
factorial74:
	mov ebx, 1
	cmp eax, 1
	je .return
	push eax
	dec eax
	call factorial74
	pop eax
	imul ebx, eax
.return:
	ret

fact78:                     ; Example 78：enter / leave，參數經由 stack
	push ebx
	push dword [esp+8]
	call factorial78
	add esp, 4
	mov eax, ebx
	pop ebx
	ret
factorial78:
	mov eax, [esp+4]
	enter 8, 0
	mov ebx, 1
	cmp eax, 1
	je .return
	mov [esp+4], eax
	dec eax
	mov [esp], eax
	call factorial78
	mov eax, [esp+4]
	imul ebx, eax
.return:
	leave
	ret

cf_after_sub:               ; 0 - 5 之後 CF 是多少（x86：借位時 CF = 1）
	mov eax, 0
	sub eax, 5
	setc al
	movzx eax, al
	ret
