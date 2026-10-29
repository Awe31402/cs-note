// locks.c：影片的銀行帳戶 [13:25]：T 個 thread 一起加同一個 balance，每個加 M 次
//   none：不保護（race）；xchg：影片 [21:23] 的 spin lock；tax：test-and-exchange [35:25]；
//   tax+bo：再加 exponential backoff [37:11]；mutex：pthread；cas：lock-free [46:49]；faa：一個 lock xadd
//   gcc -O2 -pthread locks.c -o locks && ./locks
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define M 1000000L
static volatile long balance __attribute__((aligned(64)));
static int lockv __attribute__((aligned(64)));
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static long cas_fail[16] __attribute__((aligned(64)));
static int mode, T; static pthread_barrier_t bar;
static const int cpus[] = {0, 2, 4, 6, 8, 10, 12, 13};   // 6 個不同的 P-core，再加 2 個 E-core
static void lock_xchg(void) { while (__atomic_exchange_n(&lockv, 1, __ATOMIC_ACQUIRE)) ; }
static void lock_tax(void) {
    for (;;) {
        while (__atomic_load_n(&lockv, __ATOMIC_RELAXED)) __builtin_ia32_pause();   // 只讀：line 留在 S
        if (!__atomic_exchange_n(&lockv, 1, __ATOMIC_ACQUIRE)) return;
    }
}
static unsigned rnd(unsigned *s) { *s = *s * 1103515245 + 12345; return *s >> 8; }
static void lock_tax_bo(unsigned *seed) {
    unsigned lim = 4;
    for (;;) {
        while (__atomic_load_n(&lockv, __ATOMIC_RELAXED)) __builtin_ia32_pause();
        if (!__atomic_exchange_n(&lockv, 1, __ATOMIC_ACQUIRE)) return;
        for (unsigned k = rnd(seed) % lim; k; k--) __builtin_ia32_pause();          // 等 0..lim 次 pause
        if (lim < 1024) lim *= 2;                                                     // 每失敗一次，上限加倍
    }
}
static void unlock(void) { __atomic_store_n(&lockv, 0, __ATOMIC_RELEASE); }
static void *worker(void *arg) {
    long id = (long)arg; unsigned seed = id + 1;
    cpu_set_t s; CPU_ZERO(&s); CPU_SET(cpus[id], &s); sched_setaffinity(0, sizeof s, &s);
    pthread_barrier_wait(&bar);
    for (long i = 0; i < M; i++) {
        if (mode == 0) balance = balance + 1;
        else if (mode == 1) { lock_xchg(); balance = balance + 1; unlock(); }
        else if (mode == 2) { lock_tax(); balance = balance + 1; unlock(); }
        else if (mode == 3) { lock_tax_bo(&seed); balance = balance + 1; unlock(); }
        else if (mode == 4) { pthread_mutex_lock(&mtx); balance = balance + 1; pthread_mutex_unlock(&mtx); }
        else if (mode == 5) {
            long t1 = balance;
            while (!__atomic_compare_exchange_n((long *)&balance, &t1, t1 + 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED))
                cas_fail[id]++;                                   // 失敗時 t1 會被換成最新的值
        } else __atomic_fetch_add((long *)&balance, 1, __ATOMIC_SEQ_CST);
    }
    return 0;
}
int main(void) {
    const char *modes[] = {"none", "xchg", "tax", "tax+bo", "mutex", "cas", "faa"};
    int Ts[] = {1, 2, 4, 6, 8};
    printf("%-7s", "T");
    for (int m = 0; m < 7; m++) printf("%17s", modes[m]);
    printf("    （ns／次；none 後面括號是少掉的比例；cas 括號是平均重試次數）\n");
    for (int k = 0; k < 5; k++) {
        T = Ts[k]; printf("%-7d", T);
        for (int m = 0; m < 7; m++) {
            mode = m; balance = 0; memset(cas_fail, 0, sizeof cas_fail);
            pthread_t t[8]; struct timespec a, b;
            pthread_barrier_init(&bar, NULL, T);
            clock_gettime(CLOCK_MONOTONIC, &a);
            for (long i = 0; i < T; i++) pthread_create(&t[i], NULL, worker, (void *)i);
            for (int i = 0; i < T; i++) pthread_join(t[i], NULL);
            clock_gettime(CLOCK_MONOTONIC, &b);
            pthread_barrier_destroy(&bar);
            double ns = ((b.tv_sec - a.tv_sec) * 1e9 + (b.tv_nsec - a.tv_nsec)) / (double)(T * M);
            long f = 0; for (int i = 0; i < T; i++) f += cas_fail[i];
            if (m == 0) printf("%8.2f (%4.1f%%)", ns, 100.0 * (T * M - balance) / (T * M));
            else if (mode == 5) printf("%9.2f (%4.2f)", ns, (double)f / (T * M));
            else printf("%17.2f", ns);
            if (m && balance != T * M) printf("!!錯");
        }
        printf("\n");
    }
}
