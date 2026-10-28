#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/* pointer chasing：每次讀取都依賴上一次的結果，量到的是真正的存取延遲 */
static double chase(size_t bytes)
{
	size_t n = bytes / sizeof(size_t);
	size_t *a = malloc(n * sizeof(size_t)), *perm = malloc(n * sizeof(size_t));
	for (size_t i = 0; i < n; i++) perm[i] = i;
	for (size_t i = n - 1; i > 0; i--) { size_t j = rand() % (i + 1), t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
	for (size_t i = 0; i < n; i++) a[perm[i]] = perm[(i + 1) % n];   /* 隨機的環 */
	size_t p = 0, iters = 20000000;
	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (size_t i = 0; i < iters; i++) p = a[p];
	clock_gettime(CLOCK_MONOTONIC, &t1);
	if (p == (size_t)-1) puts("");
	free(a); free(perm);
	return ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / iters;
}
int main(void)
{
	size_t kb[] = { 16, 32, 256, 1024, 2048, 8192, 16384, 65536, 262144 };
	for (int i = 0; i < 9; i++)
		printf("%8zu KB : %6.2f ns\n", kb[i], chase(kb[i] * 1024));
	return 0;
}
