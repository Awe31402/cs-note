// maxlen.c：同一條 add，前面多塞幾個沒作用的 prefix（2E），看幾 byte 以上 CPU 就不認
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

static sigjmp_buf env;
static void on_ill(int s) { siglongjmp(env, s); }

int main(void)
{
	/* add qword [rax + rbx*8 + 0x11223344], 0x11223344 ; ret
	   48 = REX.W、81 = add r/m64, imm32、84 = ModR/M（Mod 10、R/M 100 → 有 SIB、4 byte disp）、D8 = SIB（scale 8、index rbx、base rax） */
	unsigned char body[] = {0x48, 0x81, 0x84, 0xD8, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11};
	unsigned char *code = mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	static long buf[1];
	signal(SIGILL, on_ill); signal(SIGSEGV, on_ill);
	for (int np = 0; np <= 5; np++) {
		memset(code, 0x2E, np);
		memcpy(code + np, body, sizeof body);
		code[np + sizeof body] = 0xC3;                   /* ret */
		buf[0] = 0;
		int s = sigsetjmp(env, 1);
		if (!s) {
			long base = (long)buf - 0x11223344;
			asm volatile("call *%2" :: "a"(base), "b"(0L), "r"(code) : "memory", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11");
			printf("%2d byte：執行成功，buf = 0x%lx\n", np + (int)sizeof body, buf[0]);
		} else
			printf("%2d byte：%s\n", np + (int)sizeof body, s == SIGILL ? "SIGILL（CPU 不認這條指令）" : "SIGSEGV");
	}
	return 0;
}
