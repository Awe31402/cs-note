// litmus4.c：課本 §9.5 的三個例子在本機（x86）各跑 100 萬次（寫法同第 34 支的 litmus3.c）
//   圖 9.52(a)（rfi）：T1：x=1; t1=x; y=1　T2：t2=y; t3=x　　問：t1=1、t2=1、t3=0？
//   圖 9.55（R→R）：T1：t1=x; t2=y　T2：y=1　T3：t3=y; x=1　　問：t1=1、t2=0、t3=1？
//   圖 9.56（R→W，LB）：T1：t1=x; y=1　T2：t2=y; x=1　　問：t1=1、t2=1？
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#define R 1000000
static volatile long *X, *Y, *T[3];
static atomic_long go;
static int test, nth;
static const int cpus[3] = { 2, 4, 6 };
static void *th(void *arg)
{
	long me = (long)arg;
	cpu_set_t s; CPU_ZERO(&s); CPU_SET(cpus[me], &s); pthread_setaffinity_np(pthread_self(), sizeof s, &s);
	for (long i = 0; i < R; i++) {
		atomic_fetch_add(&go, 1);
		while (atomic_load(&go) < nth * (i + 1)) ;
		volatile long *x = &X[i * 8], *y = &Y[i * 8];
		if (test == 0) {                                   /* 圖 9.52(a) */
			if (me == 0) { *x = 1; T[0][i] = *x; *y = 1; }
			else { T[1][i] = *y; T[2][i] = *x; }
		} else if (test == 1) {                            /* 圖 9.55 */
			if (me == 0) { T[0][i] = *x; T[1][i] = *y; }
			else if (me == 1) *y = 1;
			else { T[2][i] = *y; *x = 1; }
		} else {                                           /* 圖 9.56：LB */
			if (me == 0) { T[0][i] = *x; *y = 1; }
			else { T[1][i] = *y; *x = 1; }
		}
	}
	return 0;
}
int main(void)
{
	const char *name[3] = { "圖 9.52(a)", "圖 9.55  ", "圖 9.56  " };
	for (test = 0; test < 3; test++) {
		nth = test == 1 ? 3 : 2;
		X = calloc(R * 8, 8); Y = calloc(R * 8, 8);
		for (int k = 0; k < 3; k++) T[k] = calloc(R, 8);
		atomic_store(&go, 0);
		pthread_t t[3];
		for (long k = 0; k < nth; k++) pthread_create(&t[k], 0, th, (void *)k);
		for (int k = 0; k < nth; k++) pthread_join(t[k], 0);
		long bad = 0, part = 0;                            /* part：「差一點」的結果（只差最後一個條件） */
		for (long i = 0; i < R; i++) {
			if (test == 0) { part += T[1][i] == 1; bad += T[0][i] == 1 && T[1][i] == 1 && T[2][i] == 0; }
			if (test == 1) { part += T[0][i] == 1 && T[2][i] == 1; bad += T[0][i] == 1 && T[1][i] == 0 && T[2][i] == 1; }
			if (test == 2) { part += T[0][i] == 1 || T[1][i] == 1; bad += T[0][i] == 1 && T[1][i] == 1; }
		}
		printf("%s：差一點的輪 %7ld 次；問的那個結果 %ld 次\n", name[test], part, bad);
		free((void *)X); free((void *)Y); for (int k = 0; k < 3; k++) free((void *)T[k]);
	}
	return 0;
}
