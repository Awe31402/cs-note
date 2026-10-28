#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
/* 課本 Example 149：OpenMP 平行加總。
 * book ：照課本寫，每個 thread 一直加到 partialSums[myId * gap]；
 *        gap = 1（預設）時相鄰 thread 的格子在同一條 cache line，gap = 8 時隔開 64 bytes
 * local：每個 thread 先加到自己的區域變數，最後才寫一次 partialSums[myId]
 * heavy：每個元素多做一些整數運算，讓工作量大於記憶體頻寬的限制 */
#define SIZE (1 << 26)                    /* 6400 萬個 int = 256 MB */
#define MAXN 64
int numbers[SIZE];
volatile long partialSums[MAXN * 8];      /* volatile：逼編譯器每次真的寫回記憶體，跟課本的寫法一樣 */

static unsigned mix(unsigned x) { for (int k = 0; k < 16; k++) x = x * 2654435761u + 0x9e3779b9u; return x; }

int main(int argc, char **argv)
{
	int N = atoi(argv[1]); const char *mode = argv[2]; int gap = argc > 3 ? atoi(argv[3]) : 1;
	for (int i = 0; i < SIZE; i++) numbers[i] = i & 7;
	omp_set_num_threads(N);
	double t0 = omp_get_wtime();
	#pragma omp parallel
	{
		int myId = omp_get_thread_num();
		int startIdx = myId * (SIZE / N), endIdx = startIdx + SIZE / N;
		partialSums[myId * gap] = 0;
		if (mode[0] == 'b') {
			for (int jdx = startIdx; jdx < endIdx; jdx++) partialSums[myId * gap] += numbers[jdx];
		} else if (mode[0] == 'l') {
			long s = 0;
			for (int jdx = startIdx; jdx < endIdx; jdx++) s += numbers[jdx];
			partialSums[myId * gap] = s;
		} else {
			long s = 0;
			for (int jdx = startIdx; jdx < endIdx; jdx++) s += mix(numbers[jdx]) & 7;
			partialSums[myId * gap] = s;
		}
	}
	long result = 0;                      /* sequential section：把 partial sums 加起來 */
	for (int idx = 0; idx < N; idx++) result += partialSums[idx * gap];
	printf("%s gap=%d N=%2d: %8.1f ms  result=%ld\n", mode, gap, N, (omp_get_wtime() - t0) * 1e3, result);
	return 0;
}
