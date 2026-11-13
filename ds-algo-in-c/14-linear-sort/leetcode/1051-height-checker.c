/* LeetCode 1051 · Height Checker（1 <= heights[i] <= 100）
 * 思路：用計數排序得到「應該的順序」，再逐格比較有幾格不同。O(n + 100)。
 *       不用真的產生排好的陣列：邊走 heights 邊從 cnt 裡依序取出下一個應該的身高。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int heightChecker(int *heights, int heightsSize)
{
    int cnt[101] = {0};
    for (int i = 0; i < heightsSize; i++)
        cnt[heights[i]]++;
    int diff = 0, h = 1;
    for (int i = 0; i < heightsSize; i++) {
        while (cnt[h] == 0)
            h++; /* 下一個應該出現的身高 */
        diff += heights[i] != h;
        cnt[h]--;
    }
    return diff;
}
/* ===== 提交範圍 結束 ===== */

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

int main(void)
{
    int a[] = {1, 1, 4, 2, 1, 3};
    assert(heightChecker(a, 6) == 3);
    int b[] = {5, 1, 2, 3, 4};
    assert(heightChecker(b, 5) == 5);
    int c[] = {1, 2, 3, 4, 5};
    assert(heightChecker(c, 5) == 0);
    srand(1051);
    for (int t = 0; t < 300; t++) {
        int n = 1 + rand() % 60, x[60], y[60];
        for (int i = 0; i < n; i++)
            x[i] = y[i] = 1 + rand() % 100;
        qsort(y, (size_t)n, sizeof *y, cmp_int);
        int want = 0;
        for (int i = 0; i < n; i++)
            want += x[i] != y[i];
        assert(heightChecker(x, n) == want);
    }
    puts("1051: passed");
    return 0;
}
