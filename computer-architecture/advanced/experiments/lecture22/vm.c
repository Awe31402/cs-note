// vm.c：第 22 支 virtual memory 的幾個現象
//  1. 一個 process 的 memory map：text、data、bss、heap、stack 各在哪
//  2. fork 之後：同一個虛擬位址，兩個 process 看到不同的值（沒有 overlap）
//  3. MAP_SHARED：故意讓兩個 process 共用同一個 frame（IPC）
//  4. 第一次碰一個 page：page fault、kernel 給一個清成 0 的 frame；只讀的話不會配新的 frame
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
int g_init = 42;          /* data：有初值的全域變數 */
int g_uninit;             /* bss：沒有初值的全域變數 */
static long minflt(void) { struct rusage u; getrusage(RUSAGE_SELF, &u); return u.ru_minflt; }
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
int main(void)
{
	int local; char *heap = malloc(100);
	setvbuf(stdout, 0, _IONBF, 0);                      /* 不要緩衝，fork 時才不會印兩次 */
	printf("1. text（main）    %p\n   data（g_init）  %p\n   bss（g_uninit） %p\n   heap（malloc）  %p\n   stack（local）  %p\n",
	       (void *)main, (void *)&g_init, (void *)&g_uninit, (void *)heap, (void *)&local);
	pid_t pid = fork();
	if (pid == 0) { g_init = 7; printf("2. child ：&g_init = %p，g_init = %d\n", (void *)&g_init, g_init); exit(0); }
	waitpid(pid, 0, 0);
	printf("2. parent：&g_init = %p，g_init = %d\n", (void *)&g_init, g_init);
	int *sh = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*sh = 1;
	if (fork() == 0) { *sh = 99; exit(0); }
	wait(0);
	printf("3. MAP_SHARED：child 寫了 99，parent 讀到 %d\n", *sh);
	size_t n = 256L << 20; long np = n / 4096;              /* 256 MiB = 65536 個 page */
	for (int mode = 0; mode < 2; mode++) {
		volatile char *p = mmap(0, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		madvise((void *)p, n, MADV_NOHUGEPAGE);                    /* 只用 4 KB 的 page */
		long f0 = minflt(); double t0 = now(); long s = 0;
		for (long i = 0; i < np; i++) { if (mode) p[i * 4096] = 1; else s += p[i * 4096]; }
		double t1 = now(); long f1 = minflt();
		for (long i = 0; i < np; i++) { if (mode) p[i * 4096] = 2; else s += p[i * 4096]; }
		double t2 = now(); long f2 = minflt();
		int allzero = 1; if (!mode) for (long i = 0; i < n; i += 512) allzero &= p[i] == 0;
		printf("4. 第一次%s %ld 個 page：%ld 次 page fault，%.0f ns/page%s；第二次：%ld 次 fault，%.1f ns/page\n",
		       mode ? "寫" : "讀", np, f1 - f0, (t1 - t0) / np * 1e9, mode ? "" : (allzero ? "（讀到的全是 0）" : ""),
		       f2 - f1, (t2 - t1) / np * 1e9);
		if (!mode) { long f3 = minflt(); for (long i = 0; i < np; i++) p[i * 4096] = 3; printf("   讀過之後再寫：又 %ld 次 page fault（這時才真的配 frame）\n", minflt() - f3); }
		munmap((void *)p, n);
	}
	return 0;
}
