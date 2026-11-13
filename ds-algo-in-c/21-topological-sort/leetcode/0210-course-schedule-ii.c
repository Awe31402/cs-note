/* LeetCode 210 · Course Schedule II
 * 跟 207 一樣，但要回傳一種修課順序；有環就回傳空陣列。
 * 思路：Kahn 排出來的順序就是答案。排不滿 n 門代表有環。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int *findOrder(int numCourses, int **prerequisites, int prerequisitesSize, int *prerequisitesColSize,
               int *returnSize)
{
    (void)prerequisitesColSize;
    int n = numCourses, m = prerequisitesSize;
    int *indeg = calloc((size_t)n, sizeof *indeg), *start = calloc((size_t)n + 1, sizeof *start);
    int *adj = malloc((size_t)(m ? m : 1) * sizeof *adj), *fill = calloc((size_t)n, sizeof *fill);
    for (int i = 0; i < m; i++) {
        start[prerequisites[i][1] + 1]++;
        indeg[prerequisites[i][0]]++;
    }
    for (int u = 0; u < n; u++)
        start[u + 1] += start[u];
    for (int i = 0; i < m; i++) {
        int b = prerequisites[i][1];
        adj[start[b] + fill[b]++] = prerequisites[i][0];
    }
    int *order = malloc((size_t)n * sizeof *order), head = 0, tail = 0; /* order 兼 queue */
    for (int v = 0; v < n; v++)
        if (indeg[v] == 0)
            order[tail++] = v;
    while (head < tail) {
        int u = order[head++];
        for (int i = start[u]; i < start[u + 1]; i++)
            if (--indeg[adj[i]] == 0)
                order[tail++] = adj[i];
    }
    free(indeg);
    free(start);
    free(adj);
    free(fill);
    *returnSize = tail == n ? n : 0;
    return order;
}
/* ===== 提交範圍 結束 ===== */

static void check(int n, int e[][2], int m, int possible)
{
    int *rows[16], cols[16], k;
    for (int i = 0; i < m; i++) {
        rows[i] = e[i];
        cols[i] = 2;
    }
    int *o = findOrder(n, rows, m, cols, &k);
    if (!possible) {
        assert(k == 0);
    } else {
        assert(k == n);
        int pos[16];
        for (int i = 0; i < n; i++)
            pos[o[i]] = i;
        for (int i = 0; i < m; i++) /* 先修課 b 要排在 a 前面 */
            assert(pos[e[i][1]] < pos[e[i][0]]);
    }
    free(o);
}

int main(void)
{
    int a[][2] = {{1, 0}};
    check(2, a, 1, 1);
    int b[][2] = {{1, 0}, {2, 0}, {3, 1}, {3, 2}};
    check(4, b, 4, 1);
    check(1, NULL, 0, 1);
    int c[][2] = {{0, 1}, {1, 2}, {2, 0}};
    check(3, c, 3, 0);
    puts("0210: passed");
    return 0;
}
