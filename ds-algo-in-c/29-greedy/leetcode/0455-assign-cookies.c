/* LeetCode 455 · Assign Cookies
 * 小孩 i 需要大小 >= g[i] 的餅乾才會滿足，每人最多一塊。最多能滿足幾個小孩？
 * 思路：兩邊都排序。用「最小的、夠用的」餅乾去滿足「胃口最小的」小孩 —— 大餅乾留給胃口大的。O(n log n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

int findContentChildren(int *g, int gSize, int *s, int sSize)
{
    if (gSize == 0 || sSize == 0)
        return 0; /* 題目允許空陣列；qsort 收到 NULL 是未定義行為 */
    qsort(g, (size_t)gSize, sizeof *g, cmp_int);
    qsort(s, (size_t)sSize, sizeof *s, cmp_int);
    int child = 0;
    for (int c = 0; c < sSize && child < gSize; c++)
        if (s[c] >= g[child]) /* 這塊夠大：給目前胃口最小的小孩 */
            child++;           /* 不夠大：這塊誰都不夠，丟掉 */
    return child;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int g1[] = {1, 2, 3}, s1[] = {1, 1};
    assert(findContentChildren(g1, 3, s1, 2) == 1);
    int g2[] = {1, 2}, s2[] = {1, 2, 3};
    assert(findContentChildren(g2, 2, s2, 3) == 2);
    int g3[] = {10, 9, 8, 7}, s3[] = {5, 6, 7, 8};
    assert(findContentChildren(g3, 4, s3, 4) == 2);
    assert(findContentChildren(g1, 3, NULL, 0) == 0);
    puts("0455: passed");
    return 0;
}
