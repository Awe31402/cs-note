#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
/* SMT（Intel 叫 hyperthreading）：同一個實體核心上的兩個 hardware thread 共用一條 pipeline。
 * chain：每條 imul 都要等上一條（ILP 很低），一個 thread 用不滿 pipeline
 * indep：4 條互不相干的 imul 鏈（ILP 高），一個 thread 就快把乘法器用滿了
 * 每個 thread 都做一樣多的工作，量全部做完要多久 */
#define N 200000000L
static int ilp;
static void *work(void *arg)
{
	cpu_set_t s; CPU_ZERO(&s); CPU_SET((int)(long)arg, &s); pthread_setaffinity_np(pthread_self(), sizeof s, &s);
	unsigned long a = 3, b = 5, c = 7, d = 9;
	if (!ilp) for (long i = 0; i < N; i += 4)
		asm volatile("imul %0,%0\n\timul %0,%0\n\timul %0,%0\n\timul %0,%0" : "+r"(a));
	else for (long i = 0; i < N; i += 4)
		asm volatile("imul %0,%0\n\timul %1,%1\n\timul %2,%2\n\timul %3,%3" : "+r"(a), "+r"(b), "+r"(c), "+r"(d));
	return (void *)(a ^ b ^ c ^ d);
}
static double run(int n, int *cpus)
{
	pthread_t t[2]; struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < n; i++) pthread_create(&t[i], NULL, work, (void *)(long)cpus[i]);
	for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &t1);
	return (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
}
int main(void)
{
	int one[] = { 2 }, same[] = { 2, 3 }, diff[] = { 2, 4 };
	for (ilp = 0; ilp < 2; ilp++) {
		double t1 = run(1, one), ts = run(2, same), td = run(2, diff);
		printf("%s: 1 thread %6.0f ms | 2 threads on the same core %6.0f ms (throughput x%.2f) | on 2 cores %6.0f ms (x%.2f)\n",
		       ilp ? "indep" : "chain", t1, ts, 2 * t1 / ts, td, 2 * t1 / td);
	}
	return 0;
}
