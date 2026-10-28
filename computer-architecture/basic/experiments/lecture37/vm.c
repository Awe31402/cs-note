#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/resource.h>
/* 影片第 37 支的五件事，在 Linux 上一個一個看：
 * 1 memory map  2 同一個虛擬位址、不同的資料  3 demand paging 與 size problem
 * 4 text 是唯讀的  5 shared memory */
int inited = 10;                  /* data 區 */
int uninit;                       /* bss 區 */

static void show_maps(void)       /* 印出這個 process 的 memory map（/proc/self/maps） */
{
	char line[512];
	FILE *f = fopen("/proc/self/maps", "r");
	while (fgets(line, sizeof line, f)) fputs("    ", stdout), fputs(line, stdout);
	fclose(f);
}

static long minflt(void) { struct rusage r; getrusage(RUSAGE_SELF, &r); return r.ru_minflt; }
static long rss_kb(void) { long a, b; FILE *f = fopen("/proc/self/statm", "r"); fscanf(f, "%ld %ld", &a, &b); fclose(f); return b * 4; }
static int present(void *p)       /* /proc/self/pagemap 第 63 bit：這個 page 有沒有對應的 frame */
{
	uint64_t e = 0; int fd = open("/proc/self/pagemap", O_RDONLY);
	pread(fd, &e, 8, (uintptr_t)p / 4096 * 8); close(fd);
	return (int)(e >> 63);
}

static sigjmp_buf jb;
static void on_segv(int s) { (void)s; siglongjmp(jb, 1); }

int main(void)
{
	int local = 0;
	char *small = malloc(16), *big = malloc(64 << 20);
	printf("[1] memory map\n    text  main     = %p\n    data  &inited  = %p\n    bss   &uninit  = %p\n"
	       "    heap  malloc16 = %p\n    mmap  malloc64M= %p\n    stack &local   = %p\n",
	       (void *)main, (void *)&inited, (void *)&uninit, small, big, (void *)&local);
	show_maps();

	printf("[2] fork：同一個虛擬位址 %p\n", (void *)&inited);
	fflush(stdout);
	if (fork() == 0) { inited = 222; printf("    child : inited = %d\n", inited); fflush(stdout); _exit(0); }
	wait(NULL);
	printf("    parent: inited = %d\n", inited);

	long f0 = minflt(), r0 = rss_kb();
	size_t G = 1UL << 30;
	char *a = mmap(NULL, G, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	printf("[3] mmap 1 GiB：RSS %ld KB → %ld KB，minor faults +%ld，第 0 頁 present=%d\n",
	       r0, rss_kb(), minflt() - f0, present(a));
	for (size_t i = 0; i < G; i += 4096) a[i] = 1;
	printf("    每頁寫 1 byte 後：RSS %ld KB，minor faults +%ld（1 GiB / 4 KiB = %zu 頁），第 0 頁 present=%d\n",
	       rss_kb(), minflt() - f0, G / 4096, present(a));
	munmap(a, G);
	size_t huge = 256UL << 30;     /* 256 GiB：遠大於本機的實體記憶體 */
	char *h = mmap(NULL, huge, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (h == MAP_FAILED) puts("    256 GiB mmap failed");
	else {
		h[0] = 1; h[huge / 2] = 2; h[huge - 1] = 3;
		printf("    256 GiB 的虛擬空間：頭、中間、尾各寫 1 byte → %d %d %d，RSS 只有 %ld KB\n",
		       h[0], h[huge / 2], h[huge - 1], rss_kb());
		munmap(h, huge);
	}

	signal(SIGSEGV, on_segv);
	if (sigsetjmp(jb, 1) == 0) { *(volatile char *)main = 0; puts("[4] 寫進 text：成功？"); }
	else puts("[4] 寫進 text 區（main 的第一個 byte）→ SIGSEGV：text 的 page 是唯讀的");

	int *shm = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*shm = 1;
	if (fork() == 0) { *shm = 777; _exit(0); }
	wait(NULL);
	printf("[5] MAP_SHARED：child 寫了 777，parent 讀到 %d（兩個 page 對到同一個 frame）\n", *shm);
	return 0;
}
