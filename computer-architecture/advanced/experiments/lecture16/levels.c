// levels.c：同一段程式用 -O0／-O1／-O2／-O3 編譯，跑多快？（影片 [25:08] 邀請大家試）
// 對一個 4096 個 int 的陣列重複做：a[i] = a[i] * 12 + (i & 7)，再全部加總
#include <stdio.h>
#include <time.h>
#define N 4096
#define R 100000
static unsigned a[N];         /* unsigned：乘法溢位是有定義的 */
int main(void)
{
	struct timespec t0, t1; unsigned long sum = 0;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int r = 0; r < R; r++)
		for (int i = 0; i < N; i++) { a[i] = a[i] * 12 + (i & 7); sum += a[i]; }
	clock_gettime(CLOCK_MONOTONIC, &t1);
	printf("%.3f ns/元素（sum=%lu）\n", ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / N / R, sum);
	return 0;
}
