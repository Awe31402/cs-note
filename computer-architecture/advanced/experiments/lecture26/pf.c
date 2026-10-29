// pf.c：本機的硬體 data prefetcher 抓得到哪些 stride？
//   一條 pointer chase：每一步的位址 = 上一步讀到的值（互相相依，OOO 沒辦法提早發 load），
//   但位址剛好是固定的 stride。prefetcher 認得出這個 stride，延遲就會比 DRAM 小很多。
//   陣列 1 GiB（遠大於 L3），每個 stride 從頭走 400 萬步
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define SZ (1L << 30)
int main(void)
{
	char *buf = aligned_alloc(4096, SZ);
	long strides[] = { 64, 128, 256, 512, 1024, 2048, 4096, 8192, -64, 0 };   /* 0 = 隨機 */
	long steps = 4000000;
	for (int s = 0; s < 10; s++) {
		long st = strides[s], n = steps;
		long *idx = malloc(n * sizeof *idx);
		if (st == 0) {                                   /* 隨機：從 1600 萬條 line 裡不重複地挑 n 條（部分 Fisher-Yates） */
			long L = SZ / 64, *perm = malloc(L * sizeof *perm);
			for (long i = 0; i < L; i++) perm[i] = i;
			srand(1);
			for (long i = 0; i < n; i++) { long j = i + ((long)rand() << 16 ^ rand()) % (L - i), t = perm[i]; perm[i] = perm[j]; perm[j] = t; idx[i] = perm[i] * 64; }
			free(perm);
		}
		else for (long i = 0; i < n; i++) idx[i] = ((st > 0 ? i * st : SZ + i * st - 64) % SZ + SZ) % SZ;
		for (long i = 0; i < n - 1; i++) *(char **)(buf + idx[i]) = buf + idx[i + 1];
		*(char **)(buf + idx[n - 1]) = buf + idx[0];
		char **p = (char **)(buf + idx[0]);
		struct timespec t0, t1;
		clock_gettime(CLOCK_MONOTONIC, &t0);
		for (long i = 0; i < n; i++) p = (char **)*p;
		clock_gettime(CLOCK_MONOTONIC, &t1);
		double ns = ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / n;
		if (st) printf("stride %5ld B：%6.2f ns/步%s\n", st, ns, p ? "" : "!");
		else    printf("隨機         ：%6.2f ns/步%s\n", ns, p ? "" : "!");
		free(idx);
	}
	return 0;
}
