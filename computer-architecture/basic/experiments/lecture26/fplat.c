#include <stdio.h>
#include <x86intrin.h>
/* 一串互相依賴的 addsd／mulsd／divsd，量延遲（TSC ticks，只看比例） */
#define N 20000000L
#define CHAIN(op) do { unsigned long t = __rdtsc(); for (long i = 0; i < N; i += 4) \
	asm volatile(op " %1,%0\n\t" op " %1,%0\n\t" op " %1,%0\n\t" op " %1,%0" : "+x"(x) : "x"(y)); \
	r = (double)(__rdtsc() - t) / N; } while (0)
int main(void)
{
	double x = 1.0, y = 1.0000001, add, mul, div, r;
	CHAIN("addsd"); add = r;
	CHAIN("mulsd"); mul = r;
	CHAIN("divsd"); div = r;
	printf("addsd %.2f, mulsd %.2f, divsd %.2f ticks;  mul/add = %.1f, div/add = %.1f  (x=%g)\n",
	       add, mul, div, mul / add, div / add, x);
	return 0;
}
