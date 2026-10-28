// rename.c：同樣 1 億條 imul，三種寫法
//   chain ：每條都用上一條的結果（RAW，真相依）
//   reuse ：每對 mov＋imul 都重用 %rax（只有 WAW、WAR 假相依）
//   fresh ：4 個不同暫存器輪流用（完全沒有相依）
#include <stdio.h>
#include <time.h>

#define N 100000000L
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }

int main(void)
{
	unsigned long a = 3, b = 5, c = 7, d = 9;
	double t0 = now();
	for (long i = 0; i < N; i += 4)
		asm volatile("imul %0,%0\n\timul %0,%0\n\timul %0,%0\n\timul %0,%0" : "+r"(a));
	double t1 = now();
	for (long i = 0; i < N; i += 4)
		asm volatile("mov $3,%%rax\n\timul %%rax,%%rax\n\tmov $5,%%rax\n\timul %%rax,%%rax\n\t"
			     "mov $7,%%rax\n\timul %%rax,%%rax\n\tmov $9,%%rax\n\timul %%rax,%%rax" ::: "rax");
	double t2 = now();
	for (long i = 0; i < N; i += 4)
		asm volatile("mov $3,%0\n\timul %0,%0\n\tmov $5,%1\n\timul %1,%1\n\t"
			     "mov $7,%2\n\timul %2,%2\n\tmov $9,%3\n\timul %3,%3"
			     : "=&r"(a), "=&r"(b), "=&r"(c), "=&r"(d));
	double t3 = now();
	printf("chain : %.3f ns/imul\n", (t1 - t0) / N * 1e9);
	printf("reuse : %.3f ns/imul\n", (t2 - t1) / N * 1e9);
	printf("fresh : %.3f ns/imul   (%lu)\n", (t3 - t2) / N * 1e9, a ^ b ^ c ^ d);
	return 0;
}
