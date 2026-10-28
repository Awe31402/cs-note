// spec.c：同一個 if，資料排序過（好猜）和亂數（猜不到）各跑一次，看 speculation 猜錯的代價
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N (1 << 20)
#define REP 50
static int a[N];

static int cmp(const void *x, const void *y) { return *(int *)x - *(int *)y; }

static double run(const char *name)
{
	struct timespec t0, t1;
	long sum = 0;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int r = 0; r < REP; r++)
		for (int i = 0; i < N; i++)
			if (a[i] >= 128)	/* 分支：值在 0–255，一半成立 */
				sum += a[i];
	clock_gettime(CLOCK_MONOTONIC, &t1);
	double ns = ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / ((double)N * REP);
	printf("%-8s %.2f ns/元素 (sum=%ld)\n", name, ns, sum);
	return ns;
}

int main(void)
{
	srand(1);
	for (int i = 0; i < N; i++)
		a[i] = rand() % 256;
	double u = run("亂數");
	qsort(a, N, sizeof(int), cmp);
	double s = run("排序後");
	printf("亂數 / 排序後 = %.1f 倍\n", u / s);
	return 0;
}
