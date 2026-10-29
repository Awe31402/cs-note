// fence.c：一個 fence 要多久？（影片 [67:05]：「fence 大概要 200–300 個 cycle」）
//   每圈：store 到一個變數（在 L1 裡），然後 mfence／sfence／lfence／什麼都不做；量每圈多少 ns
#include <stdio.h>
#include <time.h>
#define N 50000000L
static volatile long v[8];
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
int main(void)
{
	double t0 = now();
	for (long i = 0; i < N; i++) { v[0] = i; asm volatile("" ::: "memory"); }
	double t1 = now();
	for (long i = 0; i < N; i++) { v[0] = i; asm volatile("mfence" ::: "memory"); }
	double t2 = now();
	for (long i = 0; i < N; i++) { v[0] = i; asm volatile("sfence" ::: "memory"); }
	double t3 = now();
	for (long i = 0; i < N; i++) { v[0] = i; asm volatile("lfence" ::: "memory"); }
	double t4 = now();
	for (long i = 0; i < N; i++) { v[0] = i; asm volatile("lock addl $0, (%%rsp)" ::: "memory", "cc"); }
	double t5 = now();
	printf("沒有 fence：%.2f ns/圈\nmfence    ：%.2f\nsfence    ：%.2f\nlfence    ：%.2f\nlock add  ：%.2f\n",
	       (t1 - t0) / N * 1e9, (t2 - t1) / N * 1e9, (t3 - t2) / N * 1e9, (t4 - t3) / N * 1e9, (t5 - t4) / N * 1e9);
	return 0;
}
