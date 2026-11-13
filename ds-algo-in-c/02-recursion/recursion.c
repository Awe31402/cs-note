#include "recursion.h"

#include <assert.h>

long long factorial(int n)
{
    if (n <= 1) /* base case */
        return 1;
    return n * factorial(n - 1); /* 回來之後還要乘 n → 不是尾遞迴 */
}

static long long fact_acc(int n, long long acc)
{
    if (n <= 1)
        return acc;
    return fact_acc(n - 1, acc * n); /* 回來之後什麼都不用做 → 尾遞迴 */
}

long long factorial_tail(int n)
{
    return fact_acc(n, 1);
}

int gcd(int a, int b)
{
    if (b == 0)
        return a;
    return gcd(b, a % b);
}

long long power_naive(long long x, int n)
{
    if (n == 0)
        return 1;
    return x * power_naive(x, n - 1);
}

long long power_fast(long long x, int n)
{
    if (n == 0)
        return 1;
    long long half = power_fast(x, n / 2); /* 只算一次，重複用 */
    if (n % 2 == 0)
        return half * half;
    return half * half * x;
}

long long fib_naive(int n, long *calls)
{
    if (calls)
        (*calls)++;
    if (n < 2)
        return n;
    return fib_naive(n - 1, calls) + fib_naive(n - 2, calls);
}

static long long fib_memo_rec(int n, long long *memo)
{
    if (n < 2)
        return n;
    if (memo[n] != 0)
        return memo[n];
    memo[n] = fib_memo_rec(n - 1, memo) + fib_memo_rec(n - 2, memo);
    return memo[n];
}

long long fib_memo(int n)
{
    assert(n >= 0 && n <= 92);
    long long memo[93] = {0};
    return fib_memo_rec(n, memo);
}

long long fib_iter(int n)
{
    if (n == 0)
        return 0;
    long long a = 0, b = 1; /* a = fib(i-1), b = fib(i) */
    for (int i = 1; i < n; i++) { /* 算到 fib(n) 就停，不多算 fib(n+1)，否則 n = 92 會溢位 */
        long long t = a + b;
        a = b;
        b = t;
    }
    return b;
}

static void hanoi_rec(int n, char from, char to, char via, char (*moves)[2], long *k)
{
    if (n == 0)
        return;
    hanoi_rec(n - 1, from, via, to, moves, k); /* 1. 上面 n-1 個先搬到暫放柱 */
    if (moves) {                               /* 2. 最大的直接搬到目標 */
        moves[*k][0] = from;
        moves[*k][1] = to;
    }
    (*k)++;
    hanoi_rec(n - 1, via, to, from, moves, k); /* 3. n-1 個再從暫放柱搬到目標 */
}

long hanoi(int n, char from, char to, char via, char (*moves)[2])
{
    long k = 0;
    hanoi_rec(n, from, to, via, moves, &k);
    return k;
}
