#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
/* 課本 Example 154：不靠 OpenMP，用 pthreads ＋ fence 自己寫平行加總（weak memory model 也對）。
 * fence() 用 C11 的 atomic_thread_fence(memory_order_seq_cst)，在 x86 上 gcc 產生 lock orq $0,(%rsp)（和 Linux 的 smp_mb 同一招） */
#define N 8
#define SIZE (1 << 26)
static int numbers[SIZE];
static long partialSums[N];
static volatile int finished[N];

static void fence(void) { atomic_thread_fence(memory_order_seq_cst); }

static void *worker(void *arg)
{
	int myId = (int)(long)arg;                       /* getThreadId() */
	int startIdx = myId * (SIZE / N), endIdx = startIdx + SIZE / N;
	for (int jdx = startIdx; jdx < endIdx; jdx++) partialSums[myId] += numbers[jdx];
	fence();                                          /* 確保 partialSums[myId] 寫完才設 finished */
	finished[myId] = 1;
	return NULL;
}

int main(void)
{
	pthread_t t[N];
	for (int i = 0; i < SIZE; i++) numbers[i] = i & 7;
	fence();                                          /* 初始化全部完成，才開 thread */
	for (long i = 0; i < N; i++) pthread_create(&t[i], NULL, worker, (void *)i);
	int flag;
	do {                                              /* 等所有 thread 做完（不用 join） */
		flag = 1;
		for (int i = 0; i < N; i++) if (finished[i] == 0) { flag = 0; break; }
	} while (flag == 0);
	fence();                                          /* 看到 finished 之後才讀 partialSums */
	long result = 0;
	for (int idx = 0; idx < N; idx++) result += partialSums[idx];
	printf("result = %ld (expected %ld)\n", result, (long)SIZE / 8 * 28);
	return 0;
}
