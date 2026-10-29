// bank.c：影片的銀行轉帳 [00:50]–[08:46]，四種寫法比較（gcc -O2 -pthread -fgnu-tm bank.c -o bank）
//   global：一把全域鎖（沒有 disjoint access parallelism）
//   ordered：每個帳戶一把鎖，照編號小的先拿（影片 [06:11] 的照順序拿鎖）
//   unordered：每個帳戶一把鎖，先拿轉出的、再拿轉入的（可能 deadlock，[05:35]）
//   tm：GCC 的 __transaction_atomic（libitm，軟體 TM；本機 CPU 沒有 Intel TSX）
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define M 1000000
struct acct { long bal; pthread_mutex_t m; } __attribute__((aligned(64)));
static struct acct *acc;
static int N, T, mode;
static pthread_mutex_t global = PTHREAD_MUTEX_INITIALIZER;
static volatile long done[16];
static struct timespec fin[16];                    // 每個 thread 自己記下做完的時間（不受看門狗的輪詢影響）
static void *worker(void *arg)
{
	long id = (long)arg; unsigned s = id * 7919 + 1;
	cpu_set_t c; CPU_ZERO(&c); CPU_SET(2 * id, &c); sched_setaffinity(0, sizeof c, &c);   // cpu0、2、4…（不同的 P-core）
	for (long i = 0; i < M; i++) {
		int a = rand_r(&s) % N, b = rand_r(&s) % N;
		if (a == b) b = (b + 1) % N;
		if (mode == 0) { pthread_mutex_lock(&global); acc[a].bal--; acc[b].bal++; pthread_mutex_unlock(&global); }
		else if (mode == 1) {
			int lo = a < b ? a : b, hi = a < b ? b : a;
			pthread_mutex_lock(&acc[lo].m); pthread_mutex_lock(&acc[hi].m);
			acc[a].bal--; acc[b].bal++;
			pthread_mutex_unlock(&acc[hi].m); pthread_mutex_unlock(&acc[lo].m);
		} else if (mode == 2) {
			pthread_mutex_lock(&acc[a].m); pthread_mutex_lock(&acc[b].m);
			acc[a].bal--; acc[b].bal++;
			pthread_mutex_unlock(&acc[b].m); pthread_mutex_unlock(&acc[a].m);
		} else {
			__transaction_atomic { acc[a].bal--; acc[b].bal++; }
		}
		done[id] = i + 1;
	}
	clock_gettime(CLOCK_MONOTONIC, &fin[id]);
	return 0;
}
int main(int argc, char **argv)
{
	N = atoi(argv[1]); T = atoi(argv[2]); mode = atoi(argv[3]);
	acc = aligned_alloc(64, N * sizeof *acc);
	for (int i = 0; i < N; i++) { acc[i].bal = 1000; pthread_mutex_init(&acc[i].m, 0); }
	pthread_t t[16]; struct timespec x, y;
	clock_gettime(CLOCK_MONOTONIC, &x);
	for (long i = 0; i < T; i++) pthread_create(&t[i], 0, worker, (void *)i);
	long last = -1, still = 0;                         // 看門狗：每 10 ms 看一次，連續 1 秒沒進展就當作 deadlock
	for (;;) {
		usleep(10000);
		long sum = 0, all = 1; for (int i = 0; i < T; i++) { sum += done[i]; all &= done[i] == M; }
		if (all) break;
		still = sum == last ? still + 1 : 0;
		if (still >= 100) { printf("%9s", "DEADLOCK"); fflush(stdout); _exit(0); }
		last = sum;
	}
	for (int i = 0; i < T; i++) pthread_join(t[i], 0);
	y = fin[0];
	for (int i = 1; i < T; i++) if (fin[i].tv_sec > y.tv_sec || (fin[i].tv_sec == y.tv_sec && fin[i].tv_nsec > y.tv_nsec)) y = fin[i];
	long total = 0; for (int i = 0; i < N; i++) total += acc[i].bal;
	double ns = ((y.tv_sec - x.tv_sec) * 1e9 + (y.tv_nsec - x.tv_nsec)) / ((double)T * M);
	printf("%7.1f%s", ns, total == 1000L * N ? "  " : "!!");
	return 0;
}
