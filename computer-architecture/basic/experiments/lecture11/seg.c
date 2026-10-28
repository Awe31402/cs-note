#include <stdio.h>
#include <asm/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
/* 讀 6 個 segment register 的 selector；selector = index(13) | TI(1) | RPL(2) */
static __thread int tls_var = 42;
int main(void)
{
	unsigned short cs, ds, ss, es, fs, gs;
	asm("mov %%cs,%0" : "=r"(cs)); asm("mov %%ds,%0" : "=r"(ds));
	asm("mov %%ss,%0" : "=r"(ss)); asm("mov %%es,%0" : "=r"(es));
	asm("mov %%fs,%0" : "=r"(fs)); asm("mov %%gs,%0" : "=r"(gs));
	unsigned short s[] = { cs, ds, ss, es, fs, gs };
	const char *n[] = { "cs", "ds", "ss", "es", "fs", "gs" };
	for (int i = 0; i < 6; i++)
		printf("%s = 0x%04x  (GDT index %2d, TI %d, RPL %d)\n", n[i], s[i], s[i] >> 3, (s[i] >> 2) & 1, s[i] & 3);
	unsigned long fsbase;
	syscall(SYS_arch_prctl, ARCH_GET_FS, &fsbase);
	printf("fs base = 0x%lx, &tls_var = %p\n", fsbase, (void *)&tls_var);
	return 0;
}
