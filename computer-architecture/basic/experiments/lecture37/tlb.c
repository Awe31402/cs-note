#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
/* 每個 page 只碰 1 個 cache line，用 pointer chasing 隨機繞 n 個 page。
 * line 在 page 裡的位置錯開（第 i 頁用第 i % 64 條 line），
 * 所以 n 條 line 分散在不同的 set，n ≤ 768 時全部放得進 48 KB 的 L1。
 * 變慢的原因只剩 TLB：n 超過 TLB 的 entry 數，就要去 walk page table */
int main(void)
{
	size_t maxp = 1 << 15;
	char *buf = mmap(NULL, maxp * 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	madvise(buf, maxp * 4096, MADV_NOHUGEPAGE);      /* 確定用 4 KB 的 page */
	static int perm[1 << 15];
	for (size_t n = 16; n <= maxp; n *= 2) {
		for (size_t i = 0; i < n; i++) perm[i] = i;
		for (size_t i = n - 1; i > 0; i--) { size_t j = rand() % (i + 1); int t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
		#define AT(k) (buf + (size_t)perm[k] * 4096 + (perm[k] % 64) * 64)
		for (size_t i = 0; i < n; i++) *(char **)AT(i) = AT((i + 1) % n);
		char *p = AT(0);
		long iters = 20000000;
		struct timespec t0, t1;
		clock_gettime(CLOCK_MONOTONIC, &t0);
		for (long i = 0; i < iters; i++) p = *(char **)p;
		clock_gettime(CLOCK_MONOTONIC, &t1);
		printf("%6zu pages (%7zu KB of address space, %3zu KB of data): %5.2f ns per load%s\n", n, n * 4, n * 64 / 1024,
		       ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / iters, p ? "" : "!");
	}
	return 0;
}
