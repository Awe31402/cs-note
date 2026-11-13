#include "recursion.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void test_factorial(void)
{
    long long want = 1;
    for (int n = 1; n <= 20; n++) { /* 20! 是 long long 放得下的最大階乘 */
        want *= n;
        assert(factorial(n) == want);
        assert(factorial_tail(n) == want);
    }
    assert(factorial(0) == 1);
    assert(factorial_tail(0) == 1);
}

static void test_gcd(void)
{
    assert(gcd(62, 8) == 2); /* Thareja 的例子 */
    assert(gcd(8, 12) == 4); /* a < b 也可以：第一步會自動交換 */
    assert(gcd(7, 0) == 7);
    assert(gcd(17, 5) == 1);
    for (int a = 1; a <= 60; a++)
        for (int b = 1; b <= 60; b++) {
            int g = gcd(a, b);
            assert(a % g == 0 && b % g == 0);
            for (int d = g + 1; d <= a && d <= b; d++) /* 沒有更大的公因數 */
                assert(a % d != 0 || b % d != 0);
        }
}

static void test_power(void)
{
    assert(power_naive(2, 10) == 1024);
    assert(power_fast(2, 10) == 1024);
    assert(power_fast(3, 4) == 81); /* Thareja 的例子 */
    assert(power_fast(7, 0) == 1);
    for (long long x = -3; x <= 3; x++)
        for (int n = 0; n <= 30; n++)
            assert(power_fast(x, n) == power_naive(x, n));
    assert(power_fast(2, 62) == 4611686018427387904LL);
}

static void test_fib(void)
{
    long long want[] = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55};
    for (int n = 0; n <= 10; n++) {
        assert(fib_naive(n, NULL) == want[n]);
        assert(fib_memo(n) == want[n]);
        assert(fib_iter(n) == want[n]);
    }
    for (int n = 0; n <= 92; n++)
        assert(fib_memo(n) == fib_iter(n));
    assert(fib_iter(92) == 7540113804746346429LL);

    /* 暴力版呼叫次數 = 2·fib(n+1) - 1，指數成長 */
    for (int n = 0; n <= 25; n++) {
        long calls = 0;
        fib_naive(n, &calls);
        assert(calls == 2 * fib_iter(n + 1) - 1);
    }
}

/* 真的在三根柱子上模擬每一步，確認沒有大盤壓小盤 */
static void test_hanoi(void)
{
    for (int n = 1; n <= 12; n++) {
        long total = (1L << n) - 1;
        assert(hanoi(n, 'A', 'C', 'B', NULL) == total);

        char (*moves)[2] = malloc((size_t)total * sizeof *moves);
        assert(hanoi(n, 'A', 'C', 'B', moves) == total);

        int peg[3][12], top[3] = {0, 0, 0};
        for (int d = n; d >= 1; d--) /* 大的在下面 */
            peg[0][top[0]++] = d;
        for (long k = 0; k < total; k++) {
            int f = moves[k][0] - 'A', t = moves[k][1] - 'A';
            assert(top[f] > 0);
            int disk = peg[f][--top[f]];
            assert(top[t] == 0 || peg[t][top[t] - 1] > disk);
            peg[t][top[t]++] = disk;
        }
        assert(top[0] == 0 && top[1] == 0 && top[2] == n);
        free(moves);
    }
}

int main(void)
{
    test_factorial();
    test_gcd();
    test_power();
    test_fib();
    test_hanoi();
    puts("recursion: all tests passed");
    return 0;
}
