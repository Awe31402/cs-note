// rfo.c：影片 [14:43] 的「只想寫，也要先把整個 block 讀進來」
//   對一個 1 GiB 的陣列（遠大於 L3）只做寫入，比三種寫法：
//   normal：一般的 store，每次 8 byte（cache 要先把 block 讀進來，才能寫）
//   nt    ：non-temporal store（movnti），繞過 cache、不讀舊的 block
//   read  ：對照組，只讀不寫
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <immintrin.h>
#define SZ (1L << 30)
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
int main(void)
{
	long *a = aligned_alloc(64, SZ), n = SZ / 8, s = 0;
	for (long i = 0; i < n; i++) a[i] = i;                 /* 先摸過一次，page 都配好 */
	for (int r = 0; r < 2; r++) {
		double t0 = now();
		for (long i = 0; i < n; i++) a[i] = i + r;
		double t1 = now();
		for (long i = 0; i < n; i++) _mm_stream_si64((long long *)&a[i], i + r);
		_mm_sfence();
		double t2 = now();
		for (long i = 0; i < n; i += 8) s += a[i];      /* 每條 line 讀一次 */
		double t3 = now();
		if (r) printf("normal store：%.2f GB/s\nnt store    ：%.2f GB/s\nread        ：%.2f GB/s（%ld）\n",
			      SZ / (t1 - t0) / 1e9, SZ / (t2 - t1) / 1e9, SZ / (t3 - t2) / 1e9, s);
	}
	return 0;
}
