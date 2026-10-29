// mlp.c：non-blocking cache 能同時等幾個 miss？（MSHR 的數量）
//   K 條互不相干的 pointer chase，每一步都是 DRAM miss（1 GiB、隨機順序）。
//   cache 是 blocking 的話，K 條要一條一條等；non-blocking 的話，K 個 miss 可以同時在路上，直到 MSHR 用完
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define LINES (1L << 24)                 /* 1600 萬條 line × 64 B = 1 GiB */
int main(void)
{
	char *buf = aligned_alloc(64, LINES * 64);
	long *perm = malloc(LINES * sizeof *perm);
	for (long i = 0; i < LINES; i++) perm[i] = i;
	srand(1);
	for (long i = LINES - 1; i > 0; i--) { long j = ((long)rand() << 16 ^ rand()) % (i + 1), t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
	for (long i = 0; i < LINES; i++) *(char **)(buf + perm[i] * 64) = buf + perm[(i + 1) % LINES] * 64;
	double t1miss = 0;
	int Ks[] = { 1, 2, 4, 6, 8, 10, 12, 14, 16, 20, 24, 32 };
	for (int ki = 0; ki < 12; ki++) {
		int K = Ks[ki];
		char **p[32];
		for (int k = 0; k < K; k++) p[k] = (char **)(buf + perm[(long)k * (LINES / K)] * 64);
		long steps = 4000000 / K;
		struct timespec t0, t1;
		clock_gettime(CLOCK_MONOTONIC, &t0);
		for (long s = 0; s < steps; s++)
			for (int k = 0; k < K; k++) p[k] = (char **)*p[k];
		clock_gettime(CLOCK_MONOTONIC, &t1);
		double ns = ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / (steps * K);
		long chk = 0; for (int k = 0; k < K; k++) chk += (long)p[k] & 1;
		if (K == 1) t1miss = ns;
		printf("K = %2d 條：平均每個 miss %6.2f ns（換算同時在路上 %.1f 個）%s\n", K, ns, t1miss / ns, chk ? "!" : "");
	}
	return 0;
}
