/* LeetCode 509 · Fibonacci Number（0 <= n <= 30）
 * 思路：遞迴式 F(n) = F(n-1) + F(n-2)。
 *       直接照寫會重複算同樣的子問題，O(φ^n)；記起來（memoization）就是 O(n)。
 */
#include <assert.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
static int memo[31];

static int fib_rec(int n)
{
    if (n < 2)
        return n;
    if (memo[n])
        return memo[n];
    return memo[n] = fib_rec(n - 1) + fib_rec(n - 2);
}

int fib(int n)
{
    return fib_rec(n);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    assert(fib(0) == 0);
    assert(fib(1) == 1);
    assert(fib(2) == 1);
    assert(fib(3) == 2);
    assert(fib(4) == 3);
    assert(fib(30) == 832040);
    puts("0509: passed");
    return 0;
}
