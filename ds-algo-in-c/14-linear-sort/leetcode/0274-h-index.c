/* LeetCode 274 · H-Index
 * 思路：h 最大只可能是 n，所以引用數 > n 的都當成 n（桶 n）。
 *       計數後從 n 往下累加「引用 >= h 的論文數」，第一個 >= h 的 h 就是答案。O(n)，不用排序。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int hIndex(int *citations, int citationsSize)
{
    int n = citationsSize;
    int *cnt = calloc((size_t)n + 1, sizeof *cnt);
    for (int i = 0; i < n; i++)
        cnt[citations[i] > n ? n : citations[i]]++;
    int papers = 0, h = n;
    for (; h > 0; h--) {
        papers += cnt[h]; /* 引用 >= h 的論文數 */
        if (papers >= h)
            break;
    }
    free(cnt);
    return h;
}
/* ===== 提交範圍 結束 ===== */

static int brute(const int *c, int n)
{
    for (int h = n; h > 0; h--) {
        int k = 0;
        for (int i = 0; i < n; i++)
            k += c[i] >= h;
        if (k >= h)
            return h;
    }
    return 0;
}

int main(void)
{
    int a[] = {3, 0, 6, 1, 5};
    assert(hIndex(a, 5) == 3);
    int b[] = {1, 3, 1};
    assert(hIndex(b, 3) == 1);
    int c[] = {0, 0};
    assert(hIndex(c, 2) == 0);
    int d[] = {100};
    assert(hIndex(d, 1) == 1);
    srand(274);
    for (int t = 0; t < 500; t++) {
        int n = 1 + rand() % 30, x[30];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 40;
        assert(hIndex(x, n) == brute(x, n));
    }
    puts("0274: passed");
    return 0;
}
