#include <stdio.h>
#include <x86intrin.h>
/* 一長串「互相依賴」的指令：每條都要等上一條的結果，量到的是延遲（latency） */
#define N 100000000L
int main(void)
{
	unsigned long x = 3, t0, t1, t2;
	t0 = __rdtsc();
	for (long i = 0; i < N; i += 10)
		asm volatile("add %0,%0\n\tadd %0,%0\n\tadd %0,%0\n\tadd %0,%0\n\tadd %0,%0\n\t"
			     "add %0,%0\n\tadd %0,%0\n\tadd %0,%0\n\tadd %0,%0\n\tadd %0,%0" : "+r"(x));
	t1 = __rdtsc();
	for (long i = 0; i < N; i += 10)
		asm volatile("imul %0,%0\n\timul %0,%0\n\timul %0,%0\n\timul %0,%0\n\timul %0,%0\n\t"
			     "imul %0,%0\n\timul %0,%0\n\timul %0,%0\n\timul %0,%0\n\timul %0,%0" : "+r"(x));
	t2 = __rdtsc();
	printf("add : %.2f TSC ticks per instruction\n", (double)(t1 - t0) / N);
	printf("imul: %.2f TSC ticks per instruction\n", (double)(t2 - t1) / N);
	printf("imul / add = %.1f   (x=%lu)\n", (double)(t2 - t1) / (t1 - t0), x);
	return 0;
}
