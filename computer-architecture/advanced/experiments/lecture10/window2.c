// window.c：兩條互不相干的 pointer chase，中間夾 K 個 nop。
// 夾在中間的指令由 FILL 決定：nop 不用新的實體暫存器；add 每一條都要一個新的實體暫存器
// 兩個 cache miss 如果能同時進行，每圈只要等一次 miss；K 大到超過 out-of-order 視窗，就要等兩次。
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STR2(x) #x
#define STR(x) STR2(x)
#define SIZE (64 << 20)		/* 64M 個指標 = 512 MB，遠大於 L3 */
#define ITER 200000

int main(void)
{
	void **a = malloc(SIZE * sizeof(void *));
	long *perm = malloc(SIZE * sizeof(long));
	for (long i = 0; i < SIZE; i++) perm[i] = i;
	srand(1);
	for (long i = SIZE - 1; i > 0; i--) {	/* 打亂成一個大圈，每一步都 miss */
		long j = ((long)rand() << 16 ^ rand()) % (i + 1), t = perm[i];
		perm[i] = perm[j]; perm[j] = t;
	}
	for (long i = 0; i < SIZE; i++) a[perm[i]] = &a[perm[(i + 1) % SIZE]];
	void **p = &a[perm[0]], **q = &a[perm[SIZE / 2]];
	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < ITER; i++)
		asm volatile("mov (%0), %0\n\t.rept " STR(K) "\n\t" FILL "\n\t.endr\n\t"
			     "mov (%1), %1\n\t.rept " STR(K) "\n\t" FILL "\n\t.endr"
			     : "+r"(p), "+r"(q) :: "r8", "r9", "cc");
	clock_gettime(CLOCK_MONOTONIC, &t1);
	double ns = ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / ITER;
	printf("K=%4d  %6.1f ns/圈  (%p)\n", K, ns, (void *)((long)p ^ (long)q));
	return 0;
}
