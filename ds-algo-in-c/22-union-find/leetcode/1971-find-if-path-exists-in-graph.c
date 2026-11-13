/* LeetCode 1971 · Find if Path Exists in Graph
 * 思路：把每條邊的兩端 union 起來，最後看 source 和 destination 是不是同一組。
 *       O(E · α(V))。BFS/DFS 也可以，但要先建鄰接串列。
 */
#include <assert.h>
#include <stdbool.h>
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

bool validPath(int n, int **edges, int edgesSize, int *edgesColSize, int source, int destination)
{
    (void)edgesColSize;
    int *p = malloc((size_t)n * sizeof *p);
    for (int i = 0; i < n; i++)
        p[i] = i;
    for (int i = 0; i < edgesSize; i++)
        p[find(p, edges[i][0])] = find(p, edges[i][1]);
    bool ok = find(p, source) == find(p, destination);
    free(p);
    return ok;
}
/* ===== 提交範圍 結束 ===== */

static bool run(int n, int e[][2], int m, int s, int d)
{
    int *rows[8], cols[8];
    for (int i = 0; i < m; i++) {
        rows[i] = e[i];
        cols[i] = 2;
    }
    return validPath(n, rows, m, cols, s, d);
}

int main(void)
{
    int a[][2] = {{0, 1}, {1, 2}, {2, 0}};
    assert(run(3, a, 3, 0, 2));
    int b[][2] = {{0, 1}, {0, 2}, {3, 5}, {5, 4}, {4, 3}};
    assert(!run(6, b, 5, 0, 5));
    assert(run(1, NULL, 0, 0, 0)); /* 起點就是終點 */
    puts("1971: passed");
    return 0;
}
