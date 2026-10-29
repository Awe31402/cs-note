// atomics.c：影片 [39:21] 那幾種 atomic 操作，看 compiler 在 x86 和 ARM 上變成什麼指令
//   x86：gcc -O2 -S atomics.c；ARM：aarch64-linux-gnu-gcc -O2 -S（-march=armv8-a 沒有 LSE；armv8.1-a 有）
long xchg(long *p, long v) { return __atomic_exchange_n(p, v, __ATOMIC_SEQ_CST); }          // atomic exchange
int tas(char *p) { return __atomic_test_and_set(p, __ATOMIC_SEQ_CST); }                     // test and set
long faa(long *p, long v) { return __atomic_fetch_add(p, v, __ATOMIC_SEQ_CST); }            // fetch and add
int cas(long *p, long expect, long v) {                                                    // compare and set
    return __atomic_compare_exchange_n(p, &expect, v, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}
void unlock_release(int *p) { __atomic_store_n(p, 0, __ATOMIC_RELEASE); }                  // Linux 的 unlock
