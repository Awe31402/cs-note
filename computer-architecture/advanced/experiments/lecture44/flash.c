// flash.c：量 flash 儲存裝置的讀寫延遲（gcc -O2 flash.c -o flash && ./flash <檔名>）
//   O_DIRECT：繞過 OS 的 page cache，每次真的到裝置去
//   A. 隨機 4 KB 讀 2000 次；B. 隨機 4 KB 寫＋fdatasync 300 次；C. 循序 1 MB 讀
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define FSZ (512L << 20)
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e6 + t.tv_nsec / 1e3; }
static int cmp(const void *a, const void *b) { double x = *(double *)a, y = *(double *)b; return x < y ? -1 : x > y; }
static void stats(const char *nm, double *v, int n)
{
	qsort(v, n, sizeof *v, cmp);
	printf("%s：中位數 %.1f µs、p10 %.1f、p90 %.1f、p99 %.1f\n", nm, v[n / 2], v[n / 10], v[n * 9 / 10], v[n * 99 / 100]);
}
int main(int argc, char **argv)
{
	int fd = open(argv[1], O_RDWR | O_CREAT | O_DIRECT, 0644);
	char *buf; posix_memalign((void **)&buf, 4096, 1 << 20);
	memset(buf, 0x5a, 1 << 20);
	for (long off = 0; off < FSZ; off += 1 << 20) pwrite(fd, buf, 1 << 20, off);   // 先把檔案寫滿
	fdatasync(fd);
	srand(1);
	static double v[2000];
	for (int i = 0; i < 2000; i++) {
		long off = (rand() % (FSZ / 4096)) * 4096L;
		double t = now(); pread(fd, buf, 4096, off); v[i] = now() - t;
	}
	stats("A 隨機 4 KB 讀", v, 2000);
	for (int i = 0; i < 300; i++) {
		long off = (rand() % (FSZ / 4096)) * 4096L; buf[0] = i;
		double t = now(); pwrite(fd, buf, 4096, off); fdatasync(fd); v[i] = now() - t;
	}
	stats("B 隨機 4 KB 寫＋fdatasync", v, 300);
	double t = now();
	for (long off = 0; off < FSZ; off += 1 << 20) pread(fd, buf, 1 << 20, off);
	printf("C 循序讀：%.0f MB/s\n", FSZ / (now() - t));
	close(fd); unlink(argv[1]);
	return 0;
}
