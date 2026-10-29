// pred.c：影片 [41:24] 的例子：if (rand() % 2 == 0) x = y; else x = z;
//  branch：用條件分支（跟影片「沒有 predication」的版本一樣）
//  cmov  ：用 x86 的 cmov（條件搬移），沒有分支，就是 predicated execution
//  兩種輸入：random（0、1 亂數，分支猜不準）、fixed（一直是 0，分支很好猜）
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define N 10000000
static unsigned char r[N];
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static double branch(long *out)
{
	long x = 0, y = 1, z = 2, s = 0; double t0 = now();
	for (long i = 0; i < N; i++) {
		asm volatile("test $1, %1\n\tjnz 1f\n\tmov %2, %0\n\tjmp 2f\n1:\tmov %3, %0\n2:"
			     : "=&r"(x) : "r"((long)r[i]), "r"(y), "r"(z));
		s += x;
	}
	*out = s; return (now() - t0) / N * 1e9;
}
static double cmov(long *out)
{
	long x = 0, y = 1, z = 2, s = 0; double t0 = now();
	for (long i = 0; i < N; i++) {
		asm volatile("mov %2, %0\n\ttest $1, %1\n\tcmovnz %3, %0" : "=&r"(x) : "r"((long)r[i]), "r"(y), "r"(z) : "cc");
		s += x;
	}
	*out = s; return (now() - t0) / N * 1e9;
}
int main(void)
{
	long s1, s2;
	srand(1);
	for (int k = 0; k < 2; k++) {
		for (long i = 0; i < N; i++) r[i] = k == 0 ? rand() % 2 : 0;
		double b = branch(&s1), c = cmov(&s2);
		printf("%-6s：branch %.3f ns/次，cmov %.3f ns/次（結果一樣：%s）\n", k == 0 ? "random" : "fixed", b, c, s1 == s2 ? "是" : "否");
	}
	return 0;
}
