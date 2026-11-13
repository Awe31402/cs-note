/* LeetCode 1091 · Shortest Path in Binary Matrix
 * 從左上走到右下，只能走 0 的格子，可以往 8 個方向走。最短路徑經過幾格？
 * 思路：每一步權重都是 1 → BFS 第一次走到終點時的距離就是最短的。O(n²)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int shortestPathBinaryMatrix(int **grid, int gridSize, int *gridColSize)
{
    (void)gridColSize;
    int n = gridSize;
    if (grid[0][0] != 0 || grid[n - 1][n - 1] != 0)
        return -1;
    int *dist = malloc((size_t)n * n * sizeof *dist), *q = malloc((size_t)n * n * sizeof *q);
    for (int i = 0; i < n * n; i++)
        dist[i] = -1;
    int head = 0, tail = 0;
    dist[0] = 1; /* 題目算的是經過幾格，起點算 1 */
    q[tail++] = 0;
    while (head < tail) {
        int cur = q[head++], r = cur / n, c = cur % n;
        for (int dr = -1; dr <= 1; dr++)
            for (int dc = -1; dc <= 1; dc++) {
                int nr = r + dr, nc = c + dc;
                if (nr < 0 || nr >= n || nc < 0 || nc >= n || grid[nr][nc] != 0)
                    continue;
                if (dist[nr * n + nc] == -1) {
                    dist[nr * n + nc] = dist[cur] + 1;
                    q[tail++] = nr * n + nc;
                }
            }
    }
    int ans = dist[n * n - 1];
    free(dist);
    free(q);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a0[] = {0, 1}, a1[] = {1, 0};
    int *a[] = {a0, a1}, cols[] = {3, 3, 3};
    assert(shortestPathBinaryMatrix(a, 2, cols) == 2); /* 斜走一步 */
    int b0[] = {0, 0, 0}, b1[] = {1, 1, 0}, b2[] = {1, 1, 0};
    int *b[] = {b0, b1, b2};
    assert(shortestPathBinaryMatrix(b, 3, cols) == 4);
    int c0[] = {1, 0, 0}, c1[] = {1, 1, 0}, c2[] = {1, 1, 0};
    int *c[] = {c0, c1, c2};
    assert(shortestPathBinaryMatrix(c, 3, cols) == -1); /* 起點被擋住 */
    int d0[] = {0};
    int *d[] = {d0};
    assert(shortestPathBinaryMatrix(d, 1, cols) == 1);
    puts("1091: passed");
    return 0;
}
