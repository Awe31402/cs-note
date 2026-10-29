// c2c.c：本機晶片上的「網路」：任兩個核心之間，一條 cache line 換手一次要多久
//   兩個 thread 輪流寫同一條 line（ping-pong），量一個來回的時間 ÷ 2
//   如果晶片上的網路是 ring，離得越遠的核心應該越慢
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdatomic.h>
#include <time.h>
#include <stdlib.h>
#define N 300000
static _Alignas(64) atomic_int flag;
static int ca, cb;
static void pin(int c) { cpu_set_t s; CPU_ZERO(&s); CPU_SET(c, &s); pthread_setaffinity_np(pthread_self(), sizeof s, &s); }
static void *pong(void *x)
{
	pin(cb);
	for (int i = 0; i < N; i++) { while (atomic_load_explicit(&flag, memory_order_acquire) != 1); atomic_store_explicit(&flag, 0, memory_order_release); }
	return x;
}
static double pair(int a, int b)
{
	ca = a; cb = b; flag = 0;
	pthread_t t; pthread_create(&t, 0, pong, 0);
	pin(ca);
	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < N; i++) { atomic_store_explicit(&flag, 1, memory_order_release); while (atomic_load_explicit(&flag, memory_order_acquire) != 0); }
	clock_gettime(CLOCK_MONOTONIC, &t1);
	pthread_join(t, 0);
	return ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / N / 2;
}
int main(void)
{
	int cpus[] = { 0, 2, 4, 6, 8, 10, 12, 16 };
	int n = sizeof cpus / sizeof cpus[0], np = 0, pi[64], pj[64];
	double r[8][8][5];
	for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) { pi[np] = i; pj[np] = j; np++; }
	srand(1);
	for (int rep = 0; rep < 5; rep++) {             /* 每一輪用隨機的順序量所有的 pair，量 5 輪取中位數 */
		for (int k = np - 1; k > 0; k--) { int m = rand() % (k + 1), t = pi[k]; pi[k] = pi[m]; pi[m] = t; t = pj[k]; pj[k] = pj[m]; pj[m] = t; }
		for (int k = 0; k < np; k++) r[pi[k]][pj[k]][rep] = pair(cpus[pi[k]], cpus[pj[k]]);
	}
	printf("單程（ns）");
	for (int j = 0; j < n; j++) printf("  cpu%-3d", cpus[j]);
	printf("\n");
	for (int i = 0; i < n; i++) {
		printf("cpu%-6d", cpus[i]);
		for (int j = 0; j < n; j++) {
			if (j <= i) { printf("  %6s", j == i ? "-" : ""); continue; }
			double *v = r[i][j];                        /* 5 個取中位數 */
			for (int a = 0; a < 5; a++) for (int b = a + 1; b < 5; b++) if (v[b] < v[a]) { double t = v[a]; v[a] = v[b]; v[b] = t; }
			printf("  %6.1f", v[2]);
		}
		printf("\n");
	}
	return 0;
}
