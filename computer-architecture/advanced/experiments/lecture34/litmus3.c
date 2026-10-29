// litmus3.c：在本機（x86）跑三個多執行緒的例子，各 100 萬次，看「不是 SC」的結果會不會出現
//   WRC（影片 [32:25]）：T1：x=1　T2：t1=x; y=1　T3：t2=y; t3=x　　問：t1=1、t2=1、t3=0？（write 不 atomic）
//   IRIW：T1：x=1　T2：y=1　T3：t1=x; t2=y　T4：t3=y; t4=x　　問：t1=1,t2=0 而且 t3=1,t4=0？（兩個 reader 看到不同的寫入順序）
//   CoRR（影片 [16:03]，PLSC）：T1：x=1　T2：x=2　T3：t1=x; t2=x　T4：t3=x; t4=x　　問：T3 看到 1 再 2，T4 看到 2 再 1？
//   每一輪用新的變數（不同的 cache line），所有 thread 在每一輪開始前會合
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#define R 1000000
static volatile long *X, *Y, *T[4];
static atomic_long go;
static int test, nth;
static const int cpus[4] = { 2, 4, 6, 8 };
static void *th(void *arg)
{
	long me = (long)arg;
	cpu_set_t s; CPU_ZERO(&s); CPU_SET(cpus[me], &s); pthread_setaffinity_np(pthread_self(), sizeof s, &s);
	for (long i = 0; i < R; i++) {
		atomic_fetch_add(&go, 1);
		while (atomic_load(&go) < nth * (i + 1)) ;
		volatile long *x = &X[i * 8], *y = &Y[i * 8];
		if (test == 0) {                                   /* WRC */
			if (me == 0) *x = 1;
			else if (me == 1) { T[0][i] = *x; if (T[0][i]) *y = 1; }
			else { T[1][i] = *y; T[2][i] = *x; }
		} else if (test == 1) {                            /* IRIW */
			if (me == 0) *x = 1;
			else if (me == 1) *y = 1;
			else if (me == 2) { T[0][i] = *x; T[1][i] = *y; }
			else { T[2][i] = *y; T[3][i] = *x; }
		} else {                                           /* CoRR：兩個 writer、兩個 reader，同一個 x */
			if (me == 0) *x = 1;
			else if (me == 1) *x = 2;
			else if (me == 2) { T[0][i] = *x; T[1][i] = *x; }
			else { T[2][i] = *x; T[3][i] = *x; }
		}
	}
	return 0;
}
int main(void)
{
	const char *name[3] = { "WRC", "IRIW", "CoRR" };
	for (test = 0; test < 3; test++) {
		nth = test == 0 ? 3 : 4;
		X = calloc(R * 8, 8); Y = calloc(R * 8, 8);
		for (int k = 0; k < 4; k++) T[k] = calloc(R, 8);
		atomic_store(&go, 0);
		pthread_t t[4];
		for (long k = 0; k < nth; k++) pthread_create(&t[k], 0, th, (void *)k);
		for (int k = 0; k < nth; k++) pthread_join(t[k], 0);
		long bad = 0, interesting = 0;
		for (long i = 0; i < R; i++) {
			if (test == 0) { interesting += T[0][i] == 1 && T[1][i] == 1; bad += T[0][i] == 1 && T[1][i] == 1 && T[2][i] == 0; }
			if (test == 1) { interesting += (T[0][i] != T[1][i]) && (T[2][i] != T[3][i]); bad += T[0][i] == 1 && T[1][i] == 0 && T[2][i] == 1 && T[3][i] == 0; }
			if (test == 2) {
				int a = T[0][i] == 1 && T[1][i] == 2, b = T[0][i] == 2 && T[1][i] == 1;
				int c = T[2][i] == 1 && T[3][i] == 2, d = T[2][i] == 2 && T[3][i] == 1;
				interesting += (a || b) && (c || d); bad += (a && d) || (b && c);
			}
		}
		printf("%-4s：「有意思」的輪（讀者都看到了變化）%ld 次；不是 SC 的結果 %ld 次\n", name[test], interesting, bad);
		free((void *)X); free((void *)Y); for (int k = 0; k < 4; k++) free((void *)T[k]);
	}
	return 0;
}
