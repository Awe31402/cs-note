// fs.c：兩個 thread 各自一直寫「自己的」計數器，計數器隔多遠（影片 [19:50]「single writer or multiple readers」）
//   距離 8 B（同一個 64 B 的 line）→ 每次寫都要把 line 從對方搶過來（false sharing）
//   gcc -O2 -pthread fs.c -o fs && ./fs <cpuA> <cpuB>
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define N 50000000L
static char buf[4096] __attribute__((aligned(4096)));
static int dist, mode, cpu[2];          // mode 0：各寫各的（一般 store）；1：只讀；2：同一個計數器 lock add；3：各自的計數器 lock add
static pthread_barrier_t bar;
static void *worker(void *arg) {
    long id = (long)arg;
    cpu_set_t s; CPU_ZERO(&s); CPU_SET(cpu[id], &s); sched_setaffinity(0, sizeof s, &s);
    volatile long *p = (volatile long *)(buf + (mode == 2 ? 0 : id * dist));
    long sum = 0;
    pthread_barrier_wait(&bar);
    for (long i = 0; i < N; i++) {
        if (mode == 0) *p = *p + 1;
        else if (mode == 1) sum += *p;
        else __atomic_fetch_add(p, 1, __ATOMIC_SEQ_CST);   // mode 2、3：x86 的 lock add
    }
    return (void *)sum;
}
static double run(int threads) {
    pthread_t t[2]; struct timespec a, b;
    pthread_barrier_init(&bar, NULL, threads);
    for (int i = 0; i < 4096; i++) buf[i] = 0;
    clock_gettime(CLOCK_MONOTONIC, &a);
    for (long i = 0; i < threads; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < threads; i++) pthread_join(t[i], NULL);
    clock_gettime(CLOCK_MONOTONIC, &b);
    pthread_barrier_destroy(&bar);
    return ((b.tv_sec - a.tv_sec) * 1e9 + (b.tv_nsec - a.tv_nsec)) / N;
}
int main(int argc, char **argv) {
    cpu[0] = atoi(argv[1]); cpu[1] = atoi(argv[2]);
    int ds[] = {8, 32, 56, 64, 128, 256};
    for (mode = 0; mode <= 3; mode += 3) {
        const char *w = mode ? "lock add" : "一般 store";
        dist = 0; printf("[%s] 1 個 thread           ：%6.2f ns/次\n", w, run(1));
        for (int k = 0; k < 6; k++) { dist = ds[k]; printf("[%s] 2 個 thread，相距 %3d B：%6.2f ns/次\n", w, dist, run(2)); }
    }
    mode = 1; dist = 8;   printf("2 個 thread 只讀同一個 line：%6.2f ns/次\n", run(2));
    mode = 2;             printf("2 個 thread lock add 同一個：%6.2f ns/次\n", run(2));
    mode = 2;             printf("1 個 thread lock add       ：%6.2f ns/次\n", run(1));
}
