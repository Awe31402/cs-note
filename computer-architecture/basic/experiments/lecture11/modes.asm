bits 32
	mov eax, [eax]
	mov eax, [eax + ecx*2]
	mov eax, [eax + ecx*2 - 32]
	mov eax, [edx - 12]
	mov eax, [edx*2]
	mov eax, [0xFFE13342]
	mov eax, [fs:ebx]
	mov eax, 0x12345678
	add eax, 0x12345678
bits 64
	mov rax, 0x123456789A
	mov eax, [rel $ + 0x10]
