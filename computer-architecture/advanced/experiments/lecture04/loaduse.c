// loaduse.c：真的 CPU 上，相依的 add 要幾個 cycle？load 之後馬上用（load-use）要幾個 cycle？
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#define N 200000000L
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }

int main(void)
{
	static void *ring[512];                  /* 4 KB：整個放得進 L1 */
	for (int i = 0; i < 512; i++) ring[i] = &ring[(i + 1) % 512];
	uint64_t a = 1; void **p = ring;
	double t0 = now();
	for (long i = 0; i < N; i += 4)
		asm volatile("add $1, %0\n\tadd $1, %0\n\tadd $1, %0\n\tadd $1, %0" : "+r"(a));
	double t1 = now();
	uint64_t b = 1, one = 1;
	for (long i = 0; i < N; i += 4)        /* 一樣是相依的 add，但加的是暫存器 */
		asm volatile("add %1, %0\n\tadd %1, %0\n\tadd %1, %0\n\tadd %1, %0" : "+r"(b) : "r"(one));
	double t1b = now();
	for (long i = 0; i < N; i += 4)        /* 每個 load 的位址是上一個 load 的結果 */
		asm volatile("mov (%0), %0\n\tmov (%0), %0\n\tmov (%0), %0\n\tmov (%0), %0" : "+r"(p));
	double t2 = now();
	uint64_t zero = 0;
	for (long i = 0; i < N; i += 4)        /* load 之後接一個 add（加暫存器）再當下一個位址 */
		asm volatile("mov (%0), %0\n\tadd %1, %0\n\tmov (%0), %0\n\tadd %1, %0\n\t"
			     "mov (%0), %0\n\tadd %1, %0\n\tmov (%0), %0\n\tadd %1, %0" : "+r"(p) : "r"(zero));
	double t3 = now();
	printf("相依的 add $1    ：%.3f ns\n", (t1 - t0) / N * 1e9);
	printf("相依的 add reg   ：%.3f ns（%lu）\n", (t1b - t1) / N * 1e9, b);
	printf("相依的 load（L1）：%.3f ns\n", (t2 - t1b) / N * 1e9);
	printf("load ＋ add      ：%.3f ns（%lu %p）\n", (t3 - t2) / N * 1e9, a, (void *)p);
	return 0;
}
