	.intel_syntax noprefix
	add rbx, [rdx + rcx*2 + 32]     # 64 位元：多一個 REX prefix 48
	add ebx, [rip + 0x100]          # 64 位元模式：Mod = 00、R/M = 101 變成 rip 相對
	add ebx, ds:[0x12345678]        # 64 位元的絕對位址只好用 SIB 表示
	mov rax, 0x1122334455667788     # 唯一能放 8 byte immediate 的 mov
	add rax, 0x11223344             # 一般指令 immediate 最多 4 byte，再做 sign extension
