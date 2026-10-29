// stfwd.c：store → load 經過記憶體的相依鏈。值從 store 轉送給 load（store-to-load forwarding）要多久？
//  same ：存 8 byte、讀同一個 8 byte（store queue 可以直接轉送）
//  idx  ：同 same，但位址用 (buf + (i&3)*8) 這種「基底＋索引」算出來
//  part ：存 4 byte、讀 8 byte（只蓋到一半，store queue 沒辦法轉送）
//  reg  ：對照組，值只在暫存器裡走（mov + add）
#include <stdio.h>
#include <time.h>
#define N 100000000L
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
int main(void)
{
	static long buf[2];
	long x = 1;
	double t0 = now();
	for (long i = 0; i < N; i++)
		asm volatile("mov %0, (%1)\n\tmov (%1), %0\n\tadd $1, %0" : "+r"(x) : "r"(buf) : "memory");
	double t1 = now();
	for (long i = 0; i < N; i++)
		asm volatile("mov %k0, (%1)\n\tmov (%1), %0\n\tadd $1, %0" : "+r"(x) : "r"(buf) : "memory");
	double t2 = now();
	for (long i = 0; i < N; i++)
		asm volatile("mov %0, (%1,%2,8)\n\tmov (%1,%2,8), %0\n\tadd $1, %0" : "+r"(x) : "r"(buf), "r"(i & 3) : "memory");
	double t3 = now();
	for (long i = 0; i < N; i++)
		asm volatile("mov %0, %%r8\n\tmov %%r8, %0\n\tadd $1, %0" : "+r"(x) : : "r8");
	double t4 = now();
	printf("same：%.3f ns/圈\npart：%.3f ns/圈\nidx ：%.3f ns/圈\nreg ：%.3f ns/圈（%ld）\n",
	       (t1 - t0) / N * 1e9, (t2 - t1) / N * 1e9, (t3 - t2) / N * 1e9, (t4 - t3) / N * 1e9, x);
	return 0;
}
