#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* 空間局部性：在 512 MB 的陣列上，每隔 stride bytes 讀 1 個 byte，讀 2^24 次。
 * stride 小於 cache block 時，好幾次讀取落在同一個 block，只有第一次要從下層搬上來 */
#define SIZE (512L << 20)
#define READS (1L << 24)
int main(void)
{
	unsigned char *a = malloc(SIZE);
	memset(a, 1, SIZE);
	for (long stride = 1; stride <= 512; stride *= 2) {
		struct timespec t0, t1;
		unsigned long sum = 0;
		clock_gettime(CLOCK_MONOTONIC, &t0);
		for (long i = 0, j = 0; i < READS; i++, j = (j + stride) & (SIZE - 1))
			sum += a[j];
		clock_gettime(CLOCK_MONOTONIC, &t1);
		double ns = ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / READS;
		printf("stride %4ld B : %6.2f ns per read   (%ld reads per 64-B block)%s\n", stride, ns,
		       stride < 64 ? 64 / stride : 1, sum == 0 ? "!" : "");
	}
	free(a);
	return 0;
}
