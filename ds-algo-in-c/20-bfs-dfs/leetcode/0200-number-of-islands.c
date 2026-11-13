/* LeetCode 200 · Number of Islands
 * 思路：掃過每一格，遇到還沒拜訪過的 '1' 就是一座新島，島數 +1，
 *       然後從這格 BFS，把整座島（上下左右相連的 '1'）標記掉。O(mn)。
 *       用 BFS + 自己的 queue：遞迴 DFS 在 300×300 全是陸地時深度可達 9 萬層。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
int numIslands(char **grid, int gridSize, int *gridColSize)
{
    int m = gridSize, n = gridColSize[0], islands = 0;
    int *q = malloc((size_t)m * n * sizeof *q);
    const int dr[] = {1, -1, 0, 0}, dc[] = {0, 0, 1, -1};
    for (int r = 0; r < m; r++)
        for (int c = 0; c < n; c++) {
            if (grid[r][c] != '1')
                continue;
            islands++;
            int head = 0, tail = 0;
            grid[r][c] = '0'; /* 放進 queue 時就標記，避免同一格被放兩次 */
            q[tail++] = r * n + c;
            while (head < tail) {
                int cur = q[head++], cr = cur / n, cc = cur % n;
                for (int k = 0; k < 4; k++) {
                    int nr = cr + dr[k], nc = cc + dc[k];
                    if (nr >= 0 && nr < m && nc >= 0 && nc < n && grid[nr][nc] == '1') {
                        grid[nr][nc] = '0';
                        q[tail++] = nr * n + nc;
                    }
                }
            }
        }
    free(q);
    return islands;
}
/* ===== 提交範圍 結束 ===== */

static int run(const char *rows[], int m)
{
    char buf[8][8], *g[8];
    int cols[8];
    for (int i = 0; i < m; i++) {
        strcpy(buf[i], rows[i]);
        g[i] = buf[i];
        cols[i] = (int)strlen(rows[i]);
    }
    return numIslands(g, m, cols);
}

int main(void)
{
    const char *a[] = {"11110", "11010", "11000", "00000"};
    assert(run(a, 4) == 1);
    const char *b[] = {"11000", "11000", "00100", "00011"};
    assert(run(b, 4) == 3);
    const char *c[] = {"101", "010", "101"}; /* 斜的不算相連 */
    assert(run(c, 3) == 5);
    const char *d[] = {"0"};
    assert(run(d, 1) == 0);
    puts("0200: passed");
    return 0;
}
