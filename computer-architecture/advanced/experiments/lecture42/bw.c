// bw.c：DRAM 的最小傳輸單位與頻寬（gcc -O2 -fopenmp bw.c -o bw && ./bw）
//   A. 每隔 stride 個 byte 讀 8 B（一個 long），看每秒「讀進來的 line 數」：stride ≤ 64 時記憶體一樣要搬整條 64 B
//   B. 1 到 8 個 thread 一起把整塊記憶體讀過一遍，看總頻寬
#define _GNU_SOURCE
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#define SZ (2UL << 30)                                 // 2 GB：遠大於 24 MB 的 L3
static double now(void) { return omp_get_wtime(); }
int main(void)
{
	long *a = mmap(0, SZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
	madvise(a, SZ, MADV_HUGEPAGE);
	memset(a, 1, SZ);
	long n = SZ / 8, sink = 0;
	int T = 6;                                         // A 用 6 個 thread（cpu0、2…10：6 個 P-core），讓頻寬是瓶頸
	printf("A：%d 個 thread\nstride(B)  讀到的 long 數   時間(ms)   有用的 GB/s   搬的 line GB/s（每個 line 算 64 B）\n", T);
	int strides[] = {8, 16, 32, 64, 128, 256, 512, 4096};
	for (int k = 0; k < 8; k++) {
		long step = strides[k] / 8, cnt = n / step;
		double best = 1e9;
		for (int r = 0; r < 3; r++) {
			double t0 = now();
			#pragma omp parallel for num_threads(T) reduction(+:sink) schedule(static)
			for (long i = 0; i < n; i += step) sink += a[i];
			double t = now() - t0; if (t < best) best = t;
		}
		long lines = strides[k] <= 64 ? SZ / 64 : cnt;
		printf("%6d   %12ld   %8.1f   %10.2f    %10.2f\n", strides[k], cnt, best * 1e3, cnt * 8 / best / 1e9, lines * 64.0 / best / 1e9);
	}
	printf("B：把 2 GB 全部讀一遍\n");
	for (int t = 1; t <= 6; t++) {
		double best = 1e9;
		for (int r = 0; r < 3; r++) {
			double t0 = now();
			#pragma omp parallel for num_threads(t) reduction(+:sink) schedule(static)
			for (long i = 0; i < n; i++) sink += a[i];
			double x = now() - t0; if (x < best) best = x;
		}
		printf("%d thread：%.2f GB/s\n", t, SZ / best / 1e9);
	}
	return sink == 42;
}
