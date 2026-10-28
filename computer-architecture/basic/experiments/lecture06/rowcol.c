#include <stdio.h>
#include <time.h>
#define N 4096
static int m[N][N];   /* C 是 row-major：m[i][j] 和 m[i][j+1] 相鄰 */

static double run(int by_row)
{
	struct timespec t0, t1;
	long sum = 0;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			sum += by_row ? m[i][j] : m[j][i];
	clock_gettime(CLOCK_MONOTONIC, &t1);
	if (sum == 42) puts("");
	return (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
}

int main(void)
{
	for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) m[i][j] = i ^ j;
	printf("&m[0][1] - &m[0][0] = %td bytes, &m[1][0] - &m[0][0] = %td bytes\n",
	       (char *)&m[0][1] - (char *)&m[0][0], (char *)&m[1][0] - (char *)&m[0][0]);
	run(1);
	printf("row order:    %.1f ms\n", run(1));
	printf("column order: %.1f ms\n", run(0));
	return 0;
}
