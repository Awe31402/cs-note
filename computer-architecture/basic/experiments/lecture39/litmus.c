#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
/* 在真的 x86 多核心上跑課本第 39 支的三個 litmus test，每個跑 TRIALS 次，數每種結果出現幾次。
 *   coh：T1: x=1; x=2        T2: t1=x; t2=x     （coherence 的 order axiom：(2,1) 不准）
 *   mp ：T1: x=1; y=1        T2: t1=y; t2=x     （課本 Example 152：SC 不准 (1,0)）
 *   sb ：T1: x=1; t1=y       T2: y=1; t2=x      （先寫後讀：SC 不准 (0,0)；write buffer 會讓它發生）
 *   sbf：同 sb，但寫和讀之間加 mfence */
#define TRIALS 2000000
static volatile int x, y;
static volatile int r1, r2;
static volatile int go[2], done[2];
static const char *test;

static void pin(int cpu) { cpu_set_t s; CPU_ZERO(&s); CPU_SET(cpu, &s); pthread_setaffinity_np(pthread_self(), sizeof s, &s); }

static void *thread(void *arg)
{
	int me = (int)(long)arg;
	unsigned seed = 12345 + me;
	pin(me ? 4 : 2);                                     /* 兩個不同的 P-core */
	for (int i = 1; i <= TRIALS; i++) {
		int wait = rand_r(&seed) % 64;
		while (go[me] != i) ;                            /* 等主程式說「開始第 i 次」 */
		for (volatile int k = 0; k < wait; k++) ;        /* 隨機晚一點開始，讓兩邊的先後每次不一樣 */
		if (!strcmp(test, "coh")) {
			if (me == 0) { x = 1; x = 2; } else { r1 = x; r2 = x; }
		} else if (!strcmp(test, "mp")) {
			if (me == 0) { x = 1; y = 1; } else { r1 = y; r2 = x; }
		} else {
			int fence = !strcmp(test, "sbf");
			if (me == 0) { x = 1; if (fence) asm volatile("mfence" ::: "memory"); r1 = y; }
			else         { y = 1; if (fence) asm volatile("mfence" ::: "memory"); r2 = x; }
		}
		done[me] = i;
	}
	return NULL;
}

int main(int argc, char **argv)
{
	test = argv[1];
	pthread_t t[2];
	long count[3][3] = { 0 };
	for (long k = 0; k < 2; k++) pthread_create(&t[k], NULL, thread, (void *)k);
	for (int i = 1; i <= TRIALS; i++) {
		x = y = 0;
		go[0] = go[1] = i;
		while (done[0] != i || done[1] != i) ;
		count[r1][r2]++;
	}
	for (int k = 0; k < 2; k++) pthread_join(t[k], NULL);
	printf("%-3s:", test);
	for (int a = 0; a < 3; a++)
		for (int b = 0; b < 3; b++)
			if (count[a][b]) printf("  (%d,%d)=%ld", a, b, count[a][b]);
	printf("\n");
	return 0;
}
