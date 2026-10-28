#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
/* 兩個 thread 輪流寫同一條 cache line（像打乒乓）：
 * A 等 flag == 0 就把它設成 1，B 等 flag == 1 就設回 0。
 * 每一次「換手」那條 line 都要從一個核心的 cache 搬到另一個（write-invalidate），量一次來回要多久 */
#define ROUNDS 2000000
static volatile int flag __attribute__((aligned(64)));
static int cpuA, cpuB;

static void pin(int c) { cpu_set_t s; CPU_ZERO(&s); CPU_SET(c, &s); pthread_setaffinity_np(pthread_self(), sizeof s, &s); }
static void *b(void *arg)
{
	(void)arg; pin(cpuB);
	for (int i = 0; i < ROUNDS; i++) { while (flag != 1) ; flag = 0; }
	return NULL;
}
int main(int argc, char **argv)
{
	cpuA = atoi(argv[1]); cpuB = atoi(argv[2]);
	pthread_t t; pthread_create(&t, NULL, b, NULL);
	pin(cpuA);
	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < ROUNDS; i++) { while (flag != 0) ; flag = 1; }
	pthread_join(t, NULL);
	clock_gettime(CLOCK_MONOTONIC, &t1);
	printf("cpu%-2d <-> cpu%-2d : %6.1f ns per round trip\n", cpuA, cpuB,
	       ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / ROUNDS);
	return 0;
}
