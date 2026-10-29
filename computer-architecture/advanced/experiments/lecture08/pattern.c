// pattern.c：一個 branch 照著長度 L 的亂數 T/N 樣式一直重複。L 小時 predictor 背得起來；L 太大就背不起來
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define N (1 << 24)
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static unsigned char pat[1 << 20];

__attribute__((noinline)) static long run(int L)
{
	long s = 0;
	for (int r = 0, i = 0; r < N; r++) {
		if (pat[i]) s += r; else s -= 1;        /* 要預測的 branch */
		if (++i == L) i = 0;
	}
	return s;
}

int main(void)
{
	srand(1);
	for (int i = 0; i < (1 << 20); i++) pat[i] = rand() & 1;
	for (int L = 2; L <= (1 << 18); L *= 2) {
		run(L);                                  /* 先跑一次讓 predictor 學 */
		double t0 = now(); long s = run(L); double t1 = now();
		printf("L = %6d：%.2f ns/次（%ld）\n", L, (t1 - t0) / N * 1e9, s & 1);
	}
	return 0;
}
