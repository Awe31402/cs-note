/* LeetCode 204 · Count Primes
 * 思路：埃拉托斯特尼篩法 (Sieve of Eratosthenes)，O(n log log n)。
 *   從 2 開始，每找到一個質數 p，就把 p*p, p*p+p, ... 標成合數。
 * 暴力：每個數都用試除法檢查到 √k，總共 O(n√n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int countPrimes(int n)
{
    if (n < 3)
        return 0; /* 題目要「小於 n」的質數 */
    char *composite = calloc((size_t)n, 1);
    int count = 0;
    for (long p = 2; p < n; p++) {
        if (composite[p])
            continue;
        count++;
        for (long m = p * p; m < n; m += p) /* 從 p*p 開始：更小的倍數已經被更小的質數標過 */
            composite[m] = 1;
    }
    free(composite);
    return count;
}
/* ===== 提交範圍 結束 ===== */

static int is_prime(int k)
{
    if (k < 2)
        return 0;
    for (int d = 2; d * d <= k; d++)
        if (k % d == 0)
            return 0;
    return 1;
}

int main(void)
{
    assert(countPrimes(10) == 4); /* 2 3 5 7 */
    assert(countPrimes(0) == 0);
    assert(countPrimes(1) == 0);
    assert(countPrimes(2) == 0);
    assert(countPrimes(3) == 1);

    int brute = 0;
    for (int n = 1; n <= 3000; n++) {
        brute += is_prime(n - 1); /* brute = 小於 n 的質數個數 */
        assert(countPrimes(n) == brute);
    }
    assert(countPrimes(1000000) == 78498);
    assert(countPrimes(5000000) == 348513); /* 題目上限 5×10^6 */
    puts("0204: passed");
    return 0;
}
