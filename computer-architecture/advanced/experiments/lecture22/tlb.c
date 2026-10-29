// tlb.c：TLB miss 的代價。在 N 個 page 裡，每個 page 用一條 cache line，隨機順序 pointer chase。
//   同樣的資料（同樣多條 cache line），一次用 4 KB 的 page，一次用 2 MB 的 huge page（MADV_HUGEPAGE）
//   cache 的負擔一樣，差的只有 TLB
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
static double run(long np, int huge)
{
	size_t n = np * 4096 + (2 << 20);
	char *raw = mmap(0, n + (2 << 20), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	char *b = (char *)(((unsigned long)raw + (2 << 20) - 1) & ~((2UL << 20) - 1));   /* 對齊 2 MB */
	madvise(b, n, huge ? MADV_HUGEPAGE : MADV_NOHUGEPAGE);
	long *perm = malloc(np * sizeof *perm);
	for (long i = 0; i < np; i++) perm[i] = i;
	srand(1);
	for (long i = np - 1; i > 0; i--) { long j = rand() % (i + 1), t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
	#define AT(k) (b + (k) * 4096 + ((((k) ^ ((k) >> 5)) & 63) * 64))   /* 第 k 個 page 裡的一條 line；offset 錯開，huge page 時 L1、L2 的 set 才不會擠在一起 */
	for (long i = 0; i < np; i++) *(char **)AT(perm[i]) = AT(perm[(i + 1) % np]);
	char **p = (char **)AT(perm[0]);
	for (long i = 0; i < 2 * np; i++) p = (char **)*p;
	long steps = 20000000; struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (long i = 0; i < steps; i++) p = (char **)*p;
	clock_gettime(CLOCK_MONOTONIC, &t1);
	if (p == 0) puts("");
	munmap(raw, n + (2 << 20)); free(perm);
	return ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / steps;
}
int main(void)
{
	long nps[] = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 };
	printf("page 數  資料（cache line）  4 KB page  2 MB page\n");
	for (int i = 0; i < 12; i++)
		printf("%6ld  %8ld KB         %6.2f ns  %6.2f ns\n", nps[i], nps[i] * 64 / 1024, run(nps[i], 0), run(nps[i], 1));
	return 0;
}
