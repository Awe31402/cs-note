/* LeetCode 70 · Climbing Stairs
 * 每次爬 1 或 2 階，爬到第 n 階有幾種方法？
 * 思路：最後一步是從 n-1 爬 1 階，或從 n-2 爬 2 階 → ways(n) = ways(n-1) + ways(n-2)。
 *       就是 Fibonacci。只需要前兩項，O(n) 時間、O(1) 空間。
 */
#include <assert.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
int climbStairs(int n)
{
    int a = 1, b = 1; /* a = ways(i-2), b = ways(i-1)；ways(0) = ways(1) = 1 */
    for (int i = 2; i <= n; i++) {
        int c = a + b;
        a = b;
        b = c;
    }
    return b;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    assert(climbStairs(1) == 1);
    assert(climbStairs(2) == 2);
    assert(climbStairs(3) == 3);
    assert(climbStairs(5) == 8);
    assert(climbStairs(45) == 1836311903); /* 題目上限，剛好在 int 範圍內 */
    puts("0070: passed");
    return 0;
}
