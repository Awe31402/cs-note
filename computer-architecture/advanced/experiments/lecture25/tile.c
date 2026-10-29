// tile.c：影片 [35:09] 的 loop tiling。N × N 的 double 矩陣乘法 C = A × B（row major）
//   naive：i、j、k 三層（B 一行一行往下讀，沒有 spatial locality）
//   tiled：六層，每次只處理 b × b 的小塊（影片 [42:30] 的程式），b = 16、32、64、128
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static int N;
static double *A, *B, *C;
#define a(i, j) A[(long)(i) * N + (j)]
#define b(i, j) B[(long)(i) * N + (j)]
#define c(i, j) C[(long)(i) * N + (j)]
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static void naive(void)
{
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			for (int k = 0; k < N; k++)
				c(i, j) += a(i, k) * b(k, j);
}
static void tiled(int T)
{
	for (int ii = 0; ii < N; ii += T)
		for (int jj = 0; jj < N; jj += T)
			for (int kk = 0; kk < N; kk += T)
				for (int i = ii; i < ii + T; i++)
					for (int j = jj; j < jj + T; j++)
						for (int k = kk; k < kk + T; k++)
							c(i, j) += a(i, k) * b(k, j);
}
int main(int argc, char **argv)
{
	N = argc > 1 ? atoi(argv[1]) : 1024;
	A = malloc(8L * N * N); B = malloc(8L * N * N); C = malloc(8L * N * N);
	for (long i = 0; i < (long)N * N; i++) { A[i] = i % 7; B[i] = i % 5; }
	memset(C, 0, 8L * N * N);
	double t0 = now(); naive(); double t1 = now();
	double ref = 0; for (long i = 0; i < (long)N * N; i += 997) ref += C[i];
	printf("N = %d（每個矩陣 %ld MiB）\nnaive      ：%.2f s\n", N, 8L * N * N >> 20, t1 - t0);
	for (int T = 16; T <= 128; T *= 2) {
		memset(C, 0, 8L * N * N);
		double s0 = now(); tiled(T); double s1 = now();
		double chk = 0; for (long i = 0; i < (long)N * N; i += 997) chk += C[i];
		printf("tiled b=%3d：%.2f s（%.1f 倍；一個 tile %ld KB；結果%s）\n", T, s1 - s0, (t1 - t0) / (s1 - s0), 8L * T * T / 1024, chk == ref ? "一樣" : "不一樣！");
	}
	return 0;
}
