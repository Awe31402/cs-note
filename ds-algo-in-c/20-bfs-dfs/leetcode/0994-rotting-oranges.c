/* LeetCode 994 · Rotting Oranges
 * 思路：多源 BFS (multi-source BFS)。一開始把「所有」爛橘子都放進 queue（第 0 分鐘），
 *       一層一層往外擴散；層數就是分鐘數。最後還有新鮮橘子就回傳 -1。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int orangesRotting(int **grid, int gridSize, int *gridColSize)
{
    int m = gridSize, n = gridColSize[0], fresh = 0, head = 0, tail = 0, minutes = 0;
    int *q = malloc((size_t)m * n * sizeof *q);
    for (int r = 0; r < m; r++)
        for (int c = 0; c < n; c++) {
            if (grid[r][c] == 2)
                q[tail++] = r * n + c;
            else if (grid[r][c] == 1)
                fresh++;
        }
    const int dr[] = {1, -1, 0, 0}, dc[] = {0, 0, 1, -1};
    while (head < tail && fresh > 0) {
        int width = tail - head; /* 這一分鐘會擴散的爛橘子 */
        for (int i = 0; i < width; i++) {
            int cur = q[head++], cr = cur / n, cc = cur % n;
            for (int k = 0; k < 4; k++) {
                int nr = cr + dr[k], nc = cc + dc[k];
                if (nr >= 0 && nr < m && nc >= 0 && nc < n && grid[nr][nc] == 1) {
                    grid[nr][nc] = 2;
                    fresh--;
                    q[tail++] = nr * n + nc;
                }
            }
        }
        minutes++;
    }
    free(q);
    return fresh == 0 ? minutes : -1;
}
/* ===== 提交範圍 結束 ===== */

static int run(int g[][3], int m)
{
    int *rows[3], cols[3] = {3, 3, 3};
    for (int i = 0; i < m; i++)
        rows[i] = g[i];
    return orangesRotting(rows, m, cols);
}

int main(void)
{
    int a[3][3] = {{2, 1, 1}, {1, 1, 0}, {0, 1, 1}};
    assert(run(a, 3) == 4);
    int b[3][3] = {{2, 1, 1}, {0, 1, 1}, {1, 0, 1}};
    assert(run(b, 3) == -1); /* 左下角那顆碰不到 */
    int c[1][3] = {{0, 2, 0}};
    assert(run(c, 1) == 0); /* 本來就沒有新鮮的 */
    int d[1][3] = {{2, 1, 2}}; /* 兩個源頭同時擴散 */
    assert(run(d, 1) == 1);
    puts("0994: passed");
    return 0;
}
