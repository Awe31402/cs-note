// disamb.c：load 前面有一個「位址很晚才算出來」的 store，load 會不會先走？
//  每圈：t = q 經過 4 次 imul（約 12 cycle）再 and $0 → store 到 base + t（位址很晚才知道）
//        然後 q = [q]（pointer chase，下一圈要用）
//  diff：store 寫另一個陣列，跟 load 的位址不同
//  same：store 寫的就是 load 要讀的那一格（真的相依，要轉送）
//  如果 load 都要等 store 的位址算好，兩種都是「12 cycle ＋ load」；如果 CPU 敢猜「不撞」，diff 只剩 load 的延遲
#include <stdio.h>
#include <time.h>
#define N 100000000L
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static double run(long *cell, long *dst)
{
	long q = (long)cell, t;
	double t0 = now();
	for (long i = 0; i < N; i++)
		asm volatile("mov %0, %1\n\timul $1, %1\n\timul $1, %1\n\timul $1, %1\n\timul $1, %1\n\tand $0, %1\n\t"
			     "mov %3, (%2,%1)\n\t"	/* store：位址 = dst + t，很晚才知道 */
			     "mov (%0), %0"		/* load：q = [q] */
			     : "+r"(q), "=&r"(t) : "r"(dst), "r"((long)cell) : "memory");
	return (now() - t0) / N * 1e9;
}
int main(void)
{
	static long cell[8], other[8];
	cell[0] = (long)cell;
	printf("diff：%.3f ns/圈\n", run(cell, other));
	printf("same：%.3f ns/圈\n", run(cell, cell));
	return 0;
}
