// ports.c：8 條互不相干的相依鏈，看每個 cycle 最多能做幾條 add、幾條 imul、幾條 load
#include <stdio.h>
#include <time.h>
#define N 50000000L
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
#define CH8(op) op " %8, %0\n\t" op " %8, %1\n\t" op " %8, %2\n\t" op " %8, %3\n\t" \
                op " %8, %4\n\t" op " %8, %5\n\t" op " %8, %6\n\t" op " %8, %7"
int main(void)
{
	long a = 1, b = 2, c = 3, d = 4, e = 5, f = 6, g = 7, h = 8, k = 3;
	static long mem[8];
	double t0 = now();
	for (long i = 0; i < N; i++)
		asm volatile(CH8("add") : "+r"(a), "+r"(b), "+r"(c), "+r"(d), "+r"(e), "+r"(f), "+r"(g), "+r"(h) : "r"(k));
	double t1 = now();
	for (long i = 0; i < N; i++)
		asm volatile(CH8("imul") : "+r"(a), "+r"(b), "+r"(c), "+r"(d), "+r"(e), "+r"(f), "+r"(g), "+r"(h) : "r"(k));
	double t2 = now();
	for (long i = 0; i < N; i++)            /* 8 個互不相干的 load（都在 L1） */
		asm volatile("mov (%8), %0\n\tmov 8(%8), %1\n\tmov 16(%8), %2\n\tmov 24(%8), %3\n\t"
			     "mov 32(%8), %4\n\tmov 40(%8), %5\n\tmov 48(%8), %6\n\tmov 56(%8), %7"
			     : "=r"(a), "=r"(b), "=r"(c), "=r"(d), "=r"(e), "=r"(f), "=r"(g), "=r"(h) : "r"(mem));
	double t3 = now();
	printf("add ：%.3f ns/條\nimul：%.3f ns/條\nload：%.3f ns/條（%ld）\n",
	       (t1 - t0) / N / 8 * 1e9, (t2 - t1) / N / 8 * 1e9, (t3 - t2) / N / 8 * 1e9, a + b + c + d + e + f + g + h);
	return 0;
}
