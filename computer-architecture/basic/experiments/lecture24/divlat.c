// divlat.c：真的 div 指令要幾個 cycle？讓每一次 div 都用到上一次的結果（相依鏈），量平均時間
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#define ITER 20000000L
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }

static double div32(uint32_t n, uint32_t d)
{
	double t0 = now();
	uint32_t a = n;
	for (long i = 0; i < ITER; i++)       /* and eax, 0 ／ or eax, n：保留相依、又讓被除數維持 n */
		asm volatile("xor %%edx, %%edx\n\tdiv %2\n\tand $0, %0\n\tor %3, %0" : "+a"(a) : "d"(0), "r"(d), "r"(n) : "cc");
	return (now() - t0) / ITER * 1e9;
}
static double div64(uint64_t n, uint64_t d)
{
	double t0 = now();
	uint64_t a = n;
	for (long i = 0; i < ITER; i++)
		asm volatile("xor %%edx, %%edx\n\tdiv %2\n\tand $0, %0\n\tor %3, %0" : "+a"(a) : "d"(0L), "r"(d), "r"(n) : "cc");
	return (now() - t0) / ITER * 1e9;
}
static double base(uint64_t n)          /* 一樣的迴圈，只是沒有 div：扣掉 and／or 的時間 */
{
	double t0 = now();
	uint64_t a = n;
	for (long i = 0; i < ITER; i++) asm volatile("and $0, %0\n\tor %1, %0" : "+r"(a) : "r"(n) : "cc");
	return (now() - t0) / ITER * 1e9;
}
static double imul64(void)
{
	double t0 = now();
	uint64_t a = 3;
	for (long i = 0; i < ITER; i++) asm volatile("imul %0, %0" : "+r"(a));
	return (now() - t0) / ITER * 1e9;
}

int main(void)
{
	double b = base(5);
	printf("（迴圈本身 and／or：%.2f ns，下面都已扣掉）\n", b);
	printf("imul 64 位元                     ：%.2f ns\n", imul64());
	printf("div 32 位元  100 / 7（商很小）    ：%.2f ns\n", div32(100, 7) - b);
	printf("div 32 位元  4294967295 / 3      ：%.2f ns\n", div32(0xFFFFFFFFu, 3) - b);
	printf("div 64 位元  100 / 7             ：%.2f ns\n", div64(100, 7) - b);
	printf("div 64 位元  2^63 / 3（商 63 位元）：%.2f ns\n", div64(1ULL << 63, 3) - b);
	printf("div 64 位元  2^63 / 2^62（商 2）  ：%.2f ns\n", div64(1ULL << 63, 1ULL << 62) - b);
	return 0;
}
