/* LeetCode 1489 · Find Critical and Pseudo-Critical Edges in Minimum Spanning Tree
 * 關鍵邊 (critical)：拿掉它，MST 總權重會變大（或圖不連通）→ 每一棵 MST 都一定有它
 * 偽關鍵邊 (pseudo-critical)：不是關鍵邊，但「強迫先選它」之後 MST 總權重不變 → 有些 MST 有它
 * 思路：先算出 MST 總權重 best。對每條邊 i 跑兩次 Kruskal：
 *   1. 跳過 i：結果 > best（或不連通）→ 關鍵
 *   2. 否則，先強迫選 i 再跑：結果 == best → 偽關鍵
 *   n <= 100、E <= 200，O(E² α(n)) 綽綽有餘。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int find(int *p, int x)
{
    while (p[x] != x) {
        p[x] = p[p[x]];
        x = p[x];
    }
    return x;
}

static int *g_w; /* qsort 的比較函式要看到權重 */

static int cmp_idx(const void *a, const void *b)
{
    int x = g_w[*(const int *)a], y = g_w[*(const int *)b];
    return (x > y) - (x < y);
}

/* 回傳 MST 總權重；skip = 不能用的邊，force = 一定要先選的邊（-1 = 無）。不連通回傳 -1 */
static int mst(int n, int **edges, const int *order, int m, int skip, int force)
{
    int *p = malloc((size_t)n * sizeof *p), used = 0, total = 0;
    for (int i = 0; i < n; i++)
        p[i] = i;
    if (force >= 0) {
        p[find(p, edges[force][0])] = find(p, edges[force][1]);
        total += edges[force][2];
        used++;
    }
    for (int k = 0; k < m && used < n - 1; k++) {
        int i = order[k];
        if (i == skip)
            continue;
        int a = find(p, edges[i][0]), b = find(p, edges[i][1]);
        if (a != b) {
            p[a] = b;
            total += edges[i][2];
            used++;
        }
    }
    free(p);
    return used == n - 1 ? total : -1;
}

int **findCriticalAndPseudoCriticalEdges(int n, int **edges, int edgesSize, int *edgesColSize, int *returnSize,
                                         int **returnColumnSizes)
{
    (void)edgesColSize;
    int m = edgesSize;
    int *order = malloc((size_t)m * sizeof *order), *w = malloc((size_t)m * sizeof *w);
    for (int i = 0; i < m; i++) {
        order[i] = i; /* 排序 index，不動原本的 edges，答案才能回報原本的編號 */
        w[i] = edges[i][2];
    }
    g_w = w;
    qsort(order, (size_t)m, sizeof *order, cmp_idx);
    int best = mst(n, edges, order, m, -1, -1);

    int **ans = malloc(2 * sizeof *ans);
    ans[0] = malloc((size_t)m * sizeof **ans);
    ans[1] = malloc((size_t)m * sizeof **ans);
    int c = 0, pc = 0;
    for (int i = 0; i < m; i++) { /* i 由小到大 → 答案自然是遞增的 */
        int without = mst(n, edges, order, m, i, -1);
        if (without < 0 || without > best)
            ans[0][c++] = i;
        else if (mst(n, edges, order, m, -1, i) == best)
            ans[1][pc++] = i;
    }
    free(order);
    free(w);
    *returnSize = 2;
    *returnColumnSizes = malloc(2 * sizeof **returnColumnSizes);
    (*returnColumnSizes)[0] = c;
    (*returnColumnSizes)[1] = pc;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(int n, int e[][3], int m, const int *crit, int nc, const int *pseudo, int np)
{
    int *rows[16], cols[16], k, *cs;
    for (int i = 0; i < m; i++) {
        rows[i] = e[i];
        cols[i] = 3;
    }
    int **r = findCriticalAndPseudoCriticalEdges(n, rows, m, cols, &k, &cs);
    assert(k == 2 && cs[0] == nc && cs[1] == np);
    for (int i = 0; i < nc; i++)
        assert(r[0][i] == crit[i]);
    for (int i = 0; i < np; i++)
        assert(r[1][i] == pseudo[i]);
    free(r[0]);
    free(r[1]);
    free(r);
    free(cs);
}

int main(void)
{
    int a[][3] = {{0, 1, 1}, {1, 2, 1}, {2, 3, 2}, {0, 3, 2}, {0, 4, 3}, {3, 4, 3}, {1, 4, 6}};
    int ac[] = {0, 1}, ap[] = {2, 3, 4, 5};
    check(5, a, 7, ac, 2, ap, 4);
    int b[][3] = {{0, 1, 1}, {1, 2, 1}, {2, 3, 1}, {0, 3, 1}}; /* 四條一樣重：都是偽關鍵 */
    int bp[] = {0, 1, 2, 3};
    check(4, b, 4, NULL, 0, bp, 4);
    puts("1489: passed");
    return 0;
}
