// litmus.c：影片 [19:21] 和 [23:21] 的兩個例子，在本機（x86）各跑 100 萬次，數每種結果出現幾次
//   MP（message passing）：T1：x = 1; y = 1;      T2：t1 = y; t2 = x;     問：會不會 t1 = 1、t2 = 0？
//   SB（store buffering）：T1：x = 1; t1 = y;     T2：y = 1; t2 = x;      問：會不會 t1 = 0、t2 = 0？
//   SB 再跑一次：store 和 load 中間加 mfence（x86 的 memory fence）
//   每一輪用一組新的 x、y（X[i]、Y[i]，一開始都是 0），不用歸零；兩個 thread 在每一輪開始前用一個計數器會合
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#define R 1000000
static volatile long *X, *Y, *T1, *T2;
static atomic_long go;
static int test;                          /* 0 = MP、1 = SB、2 = SB + mfence */
static void *th(void *arg)
{
	long me = (long)arg;
	cpu_set_t s; CPU_ZERO(&s); CPU_SET(me ? 4 : 2, &s); pthread_setaffinity_np(pthread_self(), sizeof s, &s);
	for (long i = 0; i < R; i++) {
		atomic_fetch_add(&go, 1);
		while (atomic_load(&go) < 2 * (i + 1)) ;           /* 兩個都到了這一輪才開始 */
		volatile long *x = &X[i * 8], *y = &Y[i * 8];         /* 每一輪用不同的 cache line */
		if (test == 0) {
			if (me == 0) { *x = 1; *y = 1; }
			else { T1[i] = *y; T2[i] = *x; }
		} else {
			if (me == 0) { *x = 1; if (test == 2) asm volatile("mfence" ::: "memory"); T1[i] = *y; }
			else { *y = 1; if (test == 2) asm volatile("mfence" ::: "memory"); T2[i] = *x; }
		}
	}
	return 0;
}
int main(void)
{
	const char *name[3] = { "MP（x=1; y=1 ‖ t1=y; t2=x）", "SB（x=1; t1=y ‖ y=1; t2=x）", "SB ＋ mfence" };
	for (test = 0; test < 3; test++) {
		X = calloc(R * 8, 8); Y = calloc(R * 8, 8); T1 = calloc(R, 8); T2 = calloc(R, 8);
		atomic_store(&go, 0);
		pthread_t a, b; pthread_create(&a, 0, th, (void *)0); pthread_create(&b, 0, th, (void *)1);
		pthread_join(a, 0); pthread_join(b, 0);
		long c[2][2] = { { 0 } };
		for (long i = 0; i < R; i++) c[T1[i]][T2[i]]++;
		printf("%s：(t1,t2) = (0,0) %ld 次、(0,1) %ld、(1,0) %ld、(1,1) %ld\n", name[test], c[0][0], c[0][1], c[1][0], c[1][1]);
		free((void *)X); free((void *)Y); free((void *)T1); free((void *)T2);
	}
	return 0;
}
