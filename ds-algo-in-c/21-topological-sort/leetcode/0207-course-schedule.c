/* LeetCode 207 · Course Schedule
 * prerequisites[i] = [a, b]：要修 a，得先修 b → 邊 b → a。問：修得完所有課嗎？= 圖裡沒有環嗎？
 * 思路：Kahn 拓撲排序。能排出全部 numCourses 門課就沒有環。O(V + E)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
bool canFinish(int numCourses, int **prerequisites, int prerequisitesSize, int *prerequisitesColSize)
{
    (void)prerequisitesColSize;
    int n = numCourses, m = prerequisitesSize;
    int *indeg = calloc((size_t)n, sizeof *indeg), *start = calloc((size_t)n + 1, sizeof *start);
    int *adj = malloc((size_t)(m ? m : 1) * sizeof *adj), *fill = calloc((size_t)n, sizeof *fill);
    for (int i = 0; i < m; i++) { /* CSR：先數出度 */
        start[prerequisites[i][1] + 1]++;
        indeg[prerequisites[i][0]]++;
    }
    for (int u = 0; u < n; u++)
        start[u + 1] += start[u];
    for (int i = 0; i < m; i++) {
        int b = prerequisites[i][1];
        adj[start[b] + fill[b]++] = prerequisites[i][0];
    }
    int *q = malloc((size_t)n * sizeof *q), head = 0, tail = 0;
    for (int v = 0; v < n; v++)
        if (indeg[v] == 0)
            q[tail++] = v;
    while (head < tail) {
        int u = q[head++];
        for (int i = start[u]; i < start[u + 1]; i++)
            if (--indeg[adj[i]] == 0)
                q[tail++] = adj[i];
    }
    free(indeg);
    free(start);
    free(adj);
    free(fill);
    free(q);
    return tail == n;
}
/* ===== 提交範圍 結束 ===== */

static bool run(int n, int e[][2], int m)
{
    int *rows[16], cols[16];
    for (int i = 0; i < m; i++) {
        rows[i] = e[i];
        cols[i] = 2;
    }
    return canFinish(n, rows, m, cols);
}

int main(void)
{
    int a[][2] = {{1, 0}};
    assert(run(2, a, 1));
    int b[][2] = {{1, 0}, {0, 1}};
    assert(!run(2, b, 2));
    int c[][2] = {{1, 0}, {2, 1}, {3, 2}, {1, 3}}; /* 1→2→3→1 的環，0 不在環上 */
    assert(!run(4, c, 4));
    assert(run(3, NULL, 0));
    int d[][2] = {{0, 0}}; /* 自己是自己的先修課 */
    assert(!run(1, d, 1));
    puts("0207: passed");
    return 0;
}
