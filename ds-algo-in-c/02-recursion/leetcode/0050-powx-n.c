/* LeetCode 50 · Pow(x, n)
 * 思路：快速冪。x^n = (x^(n/2))²，n 是奇數再多乘一個 x。O(log n)。
 *       n < 0 時算 (1/x)^(-n)。
 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
static double pow_rec(double x, long long n)
{
    if (n == 0)
        return 1.0;
    double half = pow_rec(x, n / 2); /* 只遞迴一次！寫兩次 pow_rec 會變回 O(n) */
    return n % 2 ? half * half * x : half * half;
}

double myPow(double x, int n)
{
    long long m = n; /* 先轉 long long：-INT_MIN 在 int 裡會溢位 */
    if (m < 0) {
        x = 1.0 / x;
        m = -m;
    }
    return pow_rec(x, m);
}
/* ===== 提交範圍 結束 ===== */

static int close(double a, double b)
{
    double d = a - b;
    if (d < 0)
        d = -d;
    double scale = b < 0 ? -b : b;
    return d <= 1e-9 * (scale > 1 ? scale : 1);
}

int main(void)
{
    assert(close(myPow(2.0, 10), 1024.0));
    assert(close(myPow(2.1, 3), 9.261));
    assert(close(myPow(2.0, -2), 0.25));
    assert(close(myPow(5.0, 0), 1.0));
    assert(close(myPow(1.0, INT_MIN), 1.0));
    assert(close(myPow(-1.0, INT_MIN), 1.0)); /* INT_MIN 是偶數 */
    assert(close(myPow(-1.0, INT_MAX), -1.0));
    assert(close(myPow(2.0, INT_MIN), 0.0));  /* 太小，變成 0 */
    assert(close(myPow(0.5, 3), 0.125));
    puts("0050: passed");
    return 0;
}
