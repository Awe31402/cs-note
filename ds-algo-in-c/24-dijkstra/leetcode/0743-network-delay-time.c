/* LeetCode 743 · Network Delay Time
 * 從節點 k 發出訊號，傳到所有節點要多久？= 從 k 出發的最短距離中「最大的」那個；有人收不到就 -1。
 * 思路：Dijkstra。n <= 100，用 O(V²) 陣列版最簡單。
 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int networkDelayTime(int **times, int timesSize, int *timesColSize, int n, int k)
{
    (void)timesColSize;
    int w[101][101]; /* 節點 1..n */
    for (int i = 1; i <= n; i++)
        for (int j = 1; j <= n; j++)
            w[i][j] = -1;
    for (int i = 0; i < timesSize; i++)
        w[times[i][0]][times[i][1]] = times[i][2];
    int dist[101], done[101] = {0};
    for (int i = 1; i <= n; i++)
        dist[i] = INT_MAX;
    dist[k] = 0;
    for (int round = 0; round < n; round++) {
        int u = -1;
        for (int v = 1; v <= n; v++)
            if (!done[v] && dist[v] != INT_MAX && (u < 0 || dist[v] < dist[u]))
                u = v;
        if (u < 0)
            break;
        done[u] = 1;
        for (int v = 1; v <= n; v++)
            if (w[u][v] >= 0 && dist[u] + w[u][v] < dist[v])
                dist[v] = dist[u] + w[u][v];
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
    assert(run(b, 1, 2, 2) == -1); /* 單向邊：2 傳不到 1 */
    int c[][3] = {{1, 2, 5}, {1, 3, 1}, {3, 2, 1}}; /* 繞路比較快 */
    assert(run(c, 3, 3, 1) == 2);
    puts("0743: passed");
    return 0;
}
