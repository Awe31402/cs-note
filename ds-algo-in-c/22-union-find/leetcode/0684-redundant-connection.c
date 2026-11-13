/* LeetCode 684 · Redundant Connection
 * 一棵樹多加了一條邊，變成剛好有一個環。找出可以拿掉的那條邊（有多條就回傳輸入中最後出現的）。
 * 思路：依序 union 每條邊。第一條「兩端已經在同一組」的邊就會形成環 → 就是答案。
 *       因為樹加一條邊只有一個環，第一條成環的邊就是環上最後出現的那條。
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

int *findRedundantConnection(int **edges, int edgesSize, int *edgesColSize, int *returnSize)
{
    (void)edgesColSize;
    int n = edgesSize; /* n 個節點、n 條邊，節點編號 1..n */
    int *p = malloc((size_t)(n + 1) * sizeof *p), *ans = malloc(2 * sizeof *ans);
    for (int i = 0; i <= n; i++)
        p[i] = i;
    for (int i = 0; i < n; i++) {
        int a = find(p, edges[i][0]), b = find(p, edges[i][1]);
        if (a == b) {
            ans[0] = edges[i][0];
            ans[1] = edges[i][1];
            break;
        }
        p[a] = b;
    }
    free(p);
    *returnSize = 2;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(int e[][2], int m, int u, int v)
{
    int *rows[8], cols[8], k;
    for (int i = 0; i < m; i++) {
        rows[i] = e[i];
        cols[i] = 2;
    }
    int *r = findRedundantConnection(rows, m, cols, &k);
    assert(k == 2 && r[0] == u && r[1] == v);
    free(r);
}

int main(void)
{
    int a[][2] = {{1, 2}, {1, 3}, {2, 3}};
    check(a, 3, 2, 3);
    int b[][2] = {{1, 2}, {2, 3}, {3, 4}, {1, 4}, {1, 5}};
    check(b, 5, 1, 4);
    puts("0684: passed");
    return 0;
}
