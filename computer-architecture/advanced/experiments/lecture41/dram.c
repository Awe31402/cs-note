// dram.c：從使用者空間看 DRAM 的兩個行為（gcc -O2 dram.c -o dram && ./dram）
//   A. row buffer：輪流讀兩個位址（每次先 clflush，一定要去 DRAM），量每次多久。
//      同一個 bank、不同 row 的兩個位址會互相把對方的 row 關掉（row conflict），比較慢（影片 [36:29] 開一整個 row）
//   B. refresh：一直 clflush＋讀同一個位址，記下特別慢的那幾次發生的時間，看它們是不是有固定週期
#define _GNU_SOURCE
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <x86intrin.h>
#define SZ (1UL << 30)                                  // 1 GB
static inline uint64_t lat(volatile char *p)
{
	_mm_clflush((void *)p); _mm_mfence();
	uint64_t t0 = __rdtsc(); _mm_lfence();
	(void)*p; _mm_lfence();
	return __rdtsc() - t0;
}
static int cmp(const void *a, const void *b) { uint64_t x = *(uint64_t *)a, y = *(uint64_t *)b; return x < y ? -1 : x > y; }
int main(void)
{
	cpu_set_t s; CPU_ZERO(&s); CPU_SET(2, &s); sched_setaffinity(0, sizeof s, &s);
	char *buf = mmap(0, SZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
	madvise(buf, SZ, MADV_HUGEPAGE);
	memset(buf, 1, SZ);
	// TSC 頻率：跟 clock_gettime 對一下
	struct timespec a, b; clock_gettime(CLOCK_MONOTONIC, &a); uint64_t c0 = __rdtsc();
	for (volatile long i = 0; i < 100000000; i++) ;
	clock_gettime(CLOCK_MONOTONIC, &b); uint64_t c1 = __rdtsc();
	double ghz = (c1 - c0) / ((b.tv_sec - a.tv_sec) * 1e9 + (b.tv_nsec - a.tv_nsec));
	printf("TSC %.3f GHz\n", ghz);
	// A：base 和 2000 個隨機位址，各輪流讀 200 次，取中位數
	srand(1); volatile char *base = buf;
	printf("A ");
	for (int k = 0; k < 2000; k++) {
		volatile char *q = buf + ((((uint64_t)rand() << 31) ^ rand()) % SZ & ~63UL);
		uint64_t t[200];
		for (int r = 0; r < 200; r++) { lat(base); t[r] = lat(q); }
		qsort(t, 200, 8, cmp);
		printf("%.1f ", t[100] / ghz);
	}
	printf("\n");
	// B：同一個位址連續 400 萬次，記下比中位數慢 40 ns 以上的那幾次的時間（ns）
	enum { N = 4000000 };
	uint64_t *ts = malloc(N * 8), *lt = malloc(N * 8);
	for (int i = 0; i < N; i++) { ts[i] = __rdtsc(); lt[i] = lat(base); }
	uint64_t *srt = malloc(N * 8); memcpy(srt, lt, N * 8); qsort(srt, N, 8, cmp);
	double med = srt[N / 2] / ghz;
	printf("B median %.1f ns p99 %.1f ns p99.9 %.1f ns span %.1f ms\n", med, srt[N / 100 * 99] / ghz, srt[N / 1000 * 999] / ghz, (ts[N - 1] - ts[0]) / ghz / 1e6);
	printf("S ");
	for (int i = 0; i < N; i++) if (lt[i] / ghz > med + 40) printf("%.0f ", (ts[i] - ts[0]) / ghz);
	printf("\n");
	return 0;
}
