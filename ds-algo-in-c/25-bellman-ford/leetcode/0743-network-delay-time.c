/* LeetCode 743 · Network Delay Time（Bellman-Ford 版）
 * 24 主題用 Dijkstra 解過。這裡用 Bellman-Ford：不用 heap、不用鄰接串列，直接掃邊的清單。
 *   O(V · E) = 100 × 6000，一樣很快。提早結束：某一輪都沒變就停。
 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
int networkDelayTime(int **times, int timesSize, int *timesColSize, int n, int k)
{
    (void)timesColSize;
    int dist[101];
    for (int i = 1; i <= n; i++)
        dist[i] = INT_MAX;
    dist[k] = 0;
    for (int round = 1; round < n; round++) {
        int changed = 0;
        for (int i = 0; i < timesSize; i++) {
            int u = times[i][0], v = times[i][1], w = times[i][2];
            if (dist[u] != INT_MAX && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                changed = 1;
            }
        }
        if (!changed)
            break;
    }
    int ans = 0;
    for (int v = 1; v <= n; v++) {
        if (dist[v] == INT_MAX)
            return -1;
        if (dist[v] > ans)
            ans = dist[v];
    }
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static int run(int e[][3], int m, int n, int k)
{
    int *rows[8], cols[8];
    for (int i = 0; i < m; i++) {
        rows[i] = e[i];
        cols[i] = 3;
    }
    return networkDelayTime(rows, m, cols, n, k);
}

int main(void)
{
    int a[][3] = {{2, 1, 1}, {2, 3, 1}, {3, 4, 1}};
    assert(run(a, 3, 4, 2) == 2);
    int b[][3] = {{1, 2, 1}};
    assert(run(b, 1, 2, 1) == 1);
    assert(run(b, 1, 2, 2) == -1);
    int c[][3] = {{1, 2, 5}, {1, 3, 1}, {3, 2, 1}};
    assert(run(c, 3, 3, 1) == 2);
    puts("0743: passed");
    return 0;
}
