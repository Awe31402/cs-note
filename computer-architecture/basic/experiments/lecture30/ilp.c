#include <stdio.h>
#include <x86intrin.h>
/* 硬體乘法器本身也是 pipeline：同樣 1 億條 imul，
 * dep：每條都要上一條的結果（RAW 相依），indep：4 條互不相干的鏈輪流做 */
#define N 100000000L
int main(void)
{
	unsigned long a = 3, b = 5, c = 7, d = 9, t0, t1, t2;
	t0 = __rdtsc();
	for (long i = 0; i < N; i += 8)
		asm volatile("imul %0,%0\n\timul %0,%0\n\timul %0,%0\n\timul %0,%0\n\t"
			     "imul %0,%0\n\timul %0,%0\n\timul %0,%0\n\timul %0,%0" : "+r"(a));
	t1 = __rdtsc();
	for (long i = 0; i < N; i += 8)
		asm volatile("imul %0,%0\n\timul %1,%1\n\timul %2,%2\n\timul %3,%3\n\t"
			     "imul %0,%0\n\timul %1,%1\n\timul %2,%2\n\timul %3,%3"
			     : "+r"(a), "+r"(b), "+r"(c), "+r"(d));
	t2 = __rdtsc();
	printf("dep  : %.2f TSC ticks per imul\n", (double)(t1 - t0) / N);
	printf("indep: %.2f TSC ticks per imul\n", (double)(t2 - t1) / N);
	printf("dep / indep = %.1f   (%lu)\n", (double)(t1 - t0) / (t2 - t1), a ^ b ^ c ^ d);
	return 0;
}
