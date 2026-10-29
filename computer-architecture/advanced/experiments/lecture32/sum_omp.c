// sum_omp.c：影片 [12:47] 的 OpenMP 程式：把一個大陣列切成 N 份，每個 thread 算一份的部分和（map），
//   最後一個 thread 把部分和加起來（reduce）。為了讓它是「算」而不是「搬記憶體」，每個元素做一點運算
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#define SIZE (1 << 24)
static double numbers[SIZE];
static double f(double x) { for (int k = 0; k < 50; k++) x = x * 0.999 + 0.001; return x; }
int main(int argc, char **argv)
{
	int N = atoi(argv[1]);
	double partialSums[64] = { 0 }, result = 0;
	for (long i = 0; i < SIZE; i++) numbers[i] = i % 100;
	double t0 = omp_get_wtime();
#pragma omp parallel num_threads(N)
	{
		int myId = omp_get_thread_num();
		long startIdx = (long)myId * SIZE / N, endIdx = startIdx + SIZE / N;
		for (long j = startIdx; j < endIdx; j++) partialSums[myId] += f(numbers[j]);
	}
	for (int i = 0; i < N; i++) result += partialSums[i];     /* 循序的部分：reduce */
	printf("%2d threads：%.3f s（sum = %.6e）\n", N, omp_get_wtime() - t0, result);
	return 0;
}
