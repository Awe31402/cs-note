// bench.c：複製 64 MB，比較一般迴圈、rep movsd（課本 Example 84 的做法）、memcpy
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
long copy_rep(int *, const int *, long);
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
__attribute__((noinline, optimize("no-tree-loop-distribute-patterns", "no-tree-vectorize")))
static void copy_loop(int *d, const int *s, long n) { for (long i = 0; i < n; i++) d[i] = s[i]; }

int main(void)
{
	long n = 16L << 20;                       /* 1600 萬個 int = 64 MB */
	int *s = malloc(n * 4), *d = malloc(n * 4);
	memset(s, 1, n * 4); memset(d, 0, n * 4);  /* 先把 page 都碰過 */
	for (int k = 0; k < 2; k++) {
		double t0 = now(); copy_loop(d, s, n);
		double t1 = now(); copy_rep(d, s, n);
		double t2 = now(); memcpy(d, s, n * 4);
		double t3 = now();
		if (k) printf("loop %.1f GB/s | rep movsd %.1f GB/s | memcpy %.1f GB/s\n",
			      n * 4 / (t1 - t0) / 1e9, n * 4 / (t2 - t1) / 1e9, n * 4 / (t3 - t2) / 1e9);
	}
	return d[n - 1] == s[n - 1] ? 0 : 1;
}
