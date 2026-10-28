#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/* 找出 L1d 的 associativity：n 個位址彼此相差 4096 bytes（= 64 個 set × 64 B），
 * 它們全部落在 L1d 的同一個 set。用 pointer chasing 輪流讀這 n 個位址：
 * n ≤ ways 時全部放得下（L1 hit），n > ways 就開始互相擠掉（去 L2 拿） */
#ifndef STEP
#define STEP 4096
#endif
int main(void)
{
	char *buf = aligned_alloc(4096, 64 * STEP);
	for (int n = 1; n <= 24; n++) {
		int order[64];
		for (int i = 0; i < n; i++) order[i] = i;
		for (int i = n - 1; i > 0; i--) { int j = rand() % (i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
		for (int i = 0; i < n; i++)          /* 每個位址存下一個位址：一個長度 n 的環 */
			*(char **)(buf + order[i] * STEP) = buf + order[(i + 1) % n] * STEP;
		char *p = buf + order[0] * STEP;
		long iters = 50000000;
		struct timespec t0, t1;
		clock_gettime(CLOCK_MONOTONIC, &t0);
		for (long i = 0; i < iters; i++) p = *(char **)p;
		clock_gettime(CLOCK_MONOTONIC, &t1);
		printf("%2d lines in one set: %.2f ns per load%s\n", n,
		       ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / iters, p ? "" : "!");
	}
	return 0;
}
