/* LeetCode 547 · Number of Provinces
 * 鄰接矩陣表示的無向圖，問有幾個連通元件。
 * 思路：集合數一開始是 n，每成功 union 一次就 -1。O(n² · α(n))。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int find(int *p, int x)
{
    while (p[x] != x) {
        p[x] = p[p[x]]; /* 路徑減半 (path halving)：一邊走一邊讓節點指向祖父，也很有效 */
        x = p[x];
    }
    return x;
}

int findCircleNum(int **isConnected, int isConnectedSize, int *isConnectedColSize)
{
    (void)isConnectedColSize;
    int n = isConnectedSize, sets = n;
    int *p = malloc((size_t)n * sizeof *p);
    for (int i = 0; i < n; i++)
        p[i] = i;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++) /* 對稱矩陣：只看上三角 */
            if (isConnected[i][j]) {
                int a = find(p, i), b = find(p, j);
                if (a != b) {
                    p[a] = b;
                    sets--;
                }
            }
    free(p);
    return sets;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a0[] = {1, 1, 0}, a1[] = {1, 1, 0}, a2[] = {0, 0, 1};
    int *a[] = {a0, a1, a2}, cols[] = {3, 3, 3};
    assert(findCircleNum(a, 3, cols) == 2);
    int b0[] = {1, 0, 0}, b1[] = {0, 1, 0}, b2[] = {0, 0, 1};
    int *b[] = {b0, b1, b2};
    assert(findCircleNum(b, 3, cols) == 3);
    int c0[] = {1, 0, 1}, c1[] = {0, 1, 1}, c2[] = {1, 1, 1}; /* 0 和 1 透過 2 相連 */
    int *c[] = {c0, c1, c2};
    assert(findCircleNum(c, 3, cols) == 1);
    puts("0547: passed");
    return 0;
}
