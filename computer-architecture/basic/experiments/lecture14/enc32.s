# enc32.s：用 32 位元模式組譯，看每條指令的 byte
	.intel_syntax noprefix
	.code32
	add ebx, [edx + ecx*2 + 32]     # 課本 Example 90：03 5C 4A 20
	add [edx + ecx*2 + 32], ebx     # 反方向：opcode 的 d bit 不同
	add ebx, ecx                    # Mod = 11：兩個都是暫存器
	add ebx, [ecx]                  # Mod = 00：沒有 displacement
	add ebx, [ecx + 8]              # Mod = 01：1 byte displacement
	add ebx, [ecx + 0x1000]         # Mod = 10：4 byte displacement
	add ebx, [esp]                  # R/M = 100 被拿去表示「後面有 SIB」→ esp 要用 SIB
	add ebx, [ebp]                  # Mod = 00、R/M = 101 被拿去表示「只有位址」→ ebp 要改用 Mod = 01、disp 0
	add ebx, ds:[0x12345678]        # 記憶體直接定址：Mod = 00、R/M = 101
	inc ecx
	lock add [ecx], ebx             # prefix F0
	rep movsd                       # prefix F3
	add bx, cx                      # prefix 66：運算元 16 位元
