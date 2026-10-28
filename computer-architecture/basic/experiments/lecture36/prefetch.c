#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/* 影片 [36:50] 的 addAll：data[vals[i]] 是「不規則存取」（index 本身來自另一個陣列），
 * 硬體 prefetcher 猜不到；在迴圈裡用 __builtin_prefetch 提前抓 DIST 圈之後要用的元素 */
#define N (1 << 25)                       /* 3200 萬個 int：data 128 MB、vals 128 MB，遠大於 L3 */
static int data[N], vals[N];
static volatile long zero = 0;

/* addall_indep：每圈的 load 互不相干，亂序執行的 CPU 可以同時發出很多個 miss
 * addall_chain：index 多 XOR 一個 (sum & m)，m 在執行時才知道是 0，
 *            所以每個 load 都要等上一個 load 回來（像這門課的 in-order pipeline） */
static long addall_indep(int dist)
{
	long sum = 0;
	for (int i = 0; i < N; i++) {
		if (dist) __builtin_prefetch(&data[vals[i + dist < N ? i + dist : i]]);
		sum += data[vals[i]];
	}
	return sum;
}

static long addall_chain(int dist)
{
	long sum = 0, m = zero;
	for (int i = 0; i < N; i++) {
		if (dist) __builtin_prefetch(&data[vals[i + dist < N ? i + dist : i]]);
		sum += data[vals[i] ^ (sum & m)];
	}
	return sum;
}

int main(void)
{
	srand(1);
	for (int i = 0; i < N; i++) { data[i] = i & 0xff; vals[i] = ((unsigned)rand() * 7919u + rand()) % N; }
	int dists[] = { 0, 1, 4, 16, 64, 100, 256, 1024 };
	for (int chain = 0; chain < 2; chain++) {
		double base = 0;
		printf(chain ? "-- each load waits for the previous one --\n" : "-- independent loads --\n");
		for (int k = 0; k < 8; k++) {
			struct timespec t0, t1;
			clock_gettime(CLOCK_MONOTONIC, &t0);
			long s = chain ? addall_chain(dists[k]) : addall_indep(dists[k]);
			clock_gettime(CLOCK_MONOTONIC, &t1);
			double ms = (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
			if (k == 0) base = ms;
			printf("prefetch distance %4d : %7.1f ms  %5.1f ns/elem  (%.2fx)  sum=%ld\n",
			       dists[k], ms, ms * 1e6 / N, base / ms, s);
		}
	}
	return 0;
}
