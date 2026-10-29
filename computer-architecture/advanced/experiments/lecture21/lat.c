// lat.c：隨機的 pointer chase，看 P-core 和 E-core 在不同的資料大小下，一次 load 要多久
//   每個元素佔一整條 64-byte 的 cache line；用隨機順序串起來，prefetcher 猜不到
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void)
{
	size_t kbs[] = { 16, 32, 48, 64, 256, 1024, 2048, 4096, 16384, 32768, 131072 };
	for (int s = 0; s < sizeof kbs / sizeof kbs[0]; s++) {
		size_t n = kbs[s] * 1024 / 64;
		char *buf = aligned_alloc(64, n * 64);
		size_t *perm = malloc(n * sizeof *perm);
		for (size_t i = 0; i < n; i++) perm[i] = i;
		srand(1);
		for (size_t i = n - 1; i > 0; i--) { size_t j = rand() % (i + 1), t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
		for (size_t i = 0; i < n; i++) *(char **)(buf + perm[i] * 64) = buf + perm[(i + 1) % n] * 64;
		char **p = (char **)(buf + perm[0] * 64);
		long steps = 20000000;
		for (long i = 0; i < n; i++) p = (char **)*p;          /* 暖身 */
		struct timespec t0, t1;
		clock_gettime(CLOCK_MONOTONIC, &t0);
		for (long i = 0; i < steps; i++) p = (char **)*p;
		clock_gettime(CLOCK_MONOTONIC, &t1);
		double ns = ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / steps;
		printf("%7zu KB：%6.2f ns/次（%p）\n", kbs[s], ns, (void *)p);
		free(buf); free(perm);
	}
	return 0;
}
