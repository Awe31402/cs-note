// perprog.c：同一台機器的 P-core 和 E-core，跑兩種程式，看「快多少」是不是固定的
//   alu  ：4 個整數運算接成一條相依鏈（a→b→c→d→a），每圈 4 個 cycle，只看時脈
//   chase：在 512 MB 裡跳來跳去的 pointer chase（每一步都 cache miss，靠記憶體）
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }

int main(int argc, char **argv)
{
	double t0, t1;
	if (!strcmp(argv[1], "alu")) {
		unsigned long a = 1, b = 2, c = 3, d = 4;
		t0 = now();
		for (long i = 0; i < 400000000L; i++)
			asm volatile("add %0,%1\n\txor %1,%2\n\tadd %2,%3\n\txor %3,%0" : "+r"(a), "+r"(b), "+r"(c), "+r"(d));
		t1 = now();
		printf("alu   %.3f s (%lu)\n", t1 - t0, a ^ b ^ c ^ d);
	} else {
		long n = 64L << 20, *next = malloc(n * sizeof(long)), *perm = malloc(n * sizeof(long));
		for (long i = 0; i < n; i++) perm[i] = i;
		srand(1);
		for (long i = n - 1; i > 0; i--) { long j = ((long)rand() << 16 ^ rand()) % (i + 1), t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
		for (long i = 0; i < n; i++) next[perm[i]] = perm[(i + 1) % n];
		long p = 0;
		t0 = now();
		for (long i = 0; i < 20000000L; i++) p = next[p];
		t1 = now();
		printf("chase %.3f s (%ld)\n", t1 - t0, p);
	}
	return 0;
}
