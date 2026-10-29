// ras.c：遞迴深度 D 的 call／ret。D 超過 return address stack（RAS）的格數，ret 就會猜錯
#include <stdio.h>
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }

__attribute__((noinline)) static int rec(int d)
{
	asm volatile("" ::: "memory");
	if (d == 0) return 0;
	int r = rec(d - 1);
	asm volatile("" ::: "memory");      /* 不讓 compiler 把遞迴變成迴圈 */
	return r + 1;
}

int main(void)
{
	int D[] = {2, 4, 8, 12, 13, 14, 15, 16, 17, 20, 24, 32, 48, 64, 128, 256};
	for (unsigned k = 0; k < sizeof D / sizeof *D; k++) {
		long calls = 0, s = 0; int reps = 20000000 / D[k];
		double t0 = now();
		for (int r = 0; r < reps; r++) { s += rec(D[k]); calls += D[k] + 1; }
		double t1 = now();
		printf("深度 %3d：%.2f ns／每次 call＋ret（%ld）\n", D[k], (t1 - t0) / calls * 1e9, s & 1);
	}
	return 0;
}
