// roofline.c：本機的 roofline（影片 [30:21]、課本 §10.6）
//   對一個 512 MB 的 float 陣列，每個元素做 k 次 x = x*a + b（2k 個 FLOP），讀 4 B、寫 4 B
//   → operational intensity = 2k / 8 = k/4 FLOP/byte；k 從 1 到 1024，畫出 GFLOPS 怎麼變
//   gcc -O3 -march=native -fopenmp roofline.c -o roofline
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#define N (1L << 27)                                   // 1.3 億個 float = 512 MB
#define BLK 64                                         // 每次處理 64 個（8 個 AVX2 向量：8 條獨立的 FMA 鏈）
static void kernel(float *x, int k)
{
	#pragma omp parallel for schedule(static)
	for (long i = 0; i < N; i += BLK) {
		float v[BLK];
		for (int e = 0; e < BLK; e++) v[e] = x[i + e];
		for (int j = 0; j < k; j++)
			for (int e = 0; e < BLK; e++) v[e] = v[e] * 0.999f + 0.001f;
		for (int e = 0; e < BLK; e++) x[i + e] = v[e];
	}
}
int main(void)
{
	float *x = aligned_alloc(64, N * sizeof(float));
	#pragma omp parallel for
	for (long i = 0; i < N; i++) x[i] = 1.0f;
	printf("threads %d\n  k   OI(FLOP/B)   GFLOPS   GB/s\n", omp_get_max_threads());
	for (int k = 1; k <= 1024; k *= 2) {
		double best = 1e9;
		for (int r = 0; r < 3; r++) { double t = omp_get_wtime(); kernel(x, k); t = omp_get_wtime() - t; if (t < best) best = t; }
		printf("%4d  %8.2f   %7.1f  %6.1f\n", k, k / 4.0, 2.0 * k * N / best / 1e9, 8.0 * N / best / 1e9);
	}
	return x[0] > 1e9;
}
