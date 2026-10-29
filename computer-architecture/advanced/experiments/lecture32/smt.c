// smt.c：hardware thread（hyperthreading）什麼時候有用？
//   兩種工作：low ILP（一條很長的相依 imul 鏈，一個 cycle 大部分的 issue slot 都閒著）
//             high ILP（8 條互不相干的 add 鏈，一個 thread 就能把 ALU 塞滿）
//   比：1 個 thread、2 個 thread 在同一個 P-core 的兩個 hardware thread（cpu2、cpu3）、2 個 thread 在不同的 P-core（cpu2、cpu4）
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <time.h>
#define N 200000000L
static int ilp;
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static void *work(void *arg)
{
	cpu_set_t s; CPU_ZERO(&s); CPU_SET((long)arg, &s); pthread_setaffinity_np(pthread_self(), sizeof s, &s);
	long a = 3, b = 5, c = 7, d = 9, e = 11, f = 13, g = 15, h = 17;
	if (!ilp) for (long i = 0; i < N; i++) asm volatile("imul %0, %0" : "+r"(a));
	else for (long i = 0; i < N; i++) asm volatile("add %8,%0\n\tadd %8,%1\n\tadd %8,%2\n\tadd %8,%3\n\tadd %8,%4\n\tadd %8,%5\n\tadd %8,%6\n\tadd %8,%7"
						     : "+r"(a), "+r"(b), "+r"(c), "+r"(d), "+r"(e), "+r"(f), "+r"(g), "+r"(h) : "r"(i));
	return (void *)(a + b + c + d + e + f + g + h);
}
static double run(int n, long c0, long c1)
{
	pthread_t t[2]; long cs[2] = { c0, c1 };
	double t0 = now();
	for (int i = 0; i < n; i++) pthread_create(&t[i], 0, work, (void *)cs[i]);
	for (int i = 0; i < n; i++) pthread_join(t[i], 0);
	return now() - t0;
}
int main(void)
{
	for (ilp = 0; ilp < 2; ilp++) {
		double one = run(1, 2, 0), same = run(2, 2, 3), diff = run(2, 2, 4);
		printf("%s：1 個 thread %.2f s；2 個 thread 同一個核心（cpu2、cpu3）%.2f s（總吞吐量 %.2f 倍）；不同核心（cpu2、cpu4）%.2f s（%.2f 倍）\n",
		       ilp ? "high ILP（8 條獨立的 add）" : "low ILP（一條相依的 imul）", one, same, 2 * one / same, diff, 2 * one / diff);
	}
	return 0;
}
