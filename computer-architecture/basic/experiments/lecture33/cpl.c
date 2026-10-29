// cpl.c：使用者程式的 CPL 是多少？執行特權指令會怎樣？
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>

static sigjmp_buf env;
static void on_sig(int s) { siglongjmp(env, s); }

int main(void)
{
	unsigned short cs;
	unsigned long fl;
	asm volatile("mov %%cs, %0" : "=r"(cs));
	asm volatile("pushfq; pop %0" : "=r"(fl));
	printf("cs = 0x%x → CPL = %d（cs 的最低 2 bit）\n", cs, cs & 3);
	printf("rflags = 0x%lx → IOPL = %lu、IF（允許 interrupt）= %lu\n", fl, (fl >> 12) & 3, (fl >> 9) & 1);
	signal(SIGSEGV, on_sig);
	const char *name[] = {"cli（關掉 interrupt）", "hlt（停住 CPU）"};
	for (int i = 0; i < 2; i++) {
		int s = sigsetjmp(env, 1);
		if (!s) {
			if (i == 0) asm volatile("cli"); else asm volatile("hlt");
			printf("%s：執行成功？\n", name[i]);
		} else
			printf("%s：被擋下來，收到 %s\n", name[i], s == SIGSEGV ? "SIGSEGV（#GP）" : "其他 signal");
	}
	return 0;
}
