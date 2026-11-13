/* LeetCode 1976 · Number of Ways to Arrive at Destination
 * 無向圖，從 0 到 n-1 的「最短路徑」有幾條？答案 mod 10^9+7。
 * 思路：Dijkstra 的同時計數。ways[v] = 以最短距離到達 v 的方法數。
 *   找到更短的路 → dist 更新，ways[v] = ways[u]（之前的都作廢）
 *   找到一樣短的路 → ways[v] += ways[u]
 *   因為 u 被拿出來時 dist[u]、ways[u] 都已確定，所以這樣算是對的。n <= 200，用 O(V²) 版。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
int countPaths(int n, int **roads, int roadsSize, int *roadsColSize)
{
    (void)roadsColSize;
    const long long MOD = 1000000007, INF = (long long)4e18;
    long long *w = malloc((size_t)n * n * sizeof *w); /* 距離可能到 10^9 × 199，要 long long */
    for (int i = 0; i < n * n; i++)
        w[i] = -1;
    for (int i = 0; i < roadsSize; i++) {
        int a = roads[i][0], b = roads[i][1];
        w[a * n + b] = w[b * n + a] = roads[i][2];
    }
    long long *dist = malloc((size_t)n * sizeof *dist), *ways = calloc((size_t)n, sizeof *ways);
    char *done = calloc((size_t)n, 1);
    for (int i = 0; i < n; i++)
        dist[i] = INF;
    dist[0] = 0;
    ways[0] = 1;
    for (int round = 0; round < n; round++) {
        int u = -1;
        for (int v = 0; v < n; v++)
            if (!done[v] && dist[v] < INF && (u < 0 || dist[v] < dist[u]))
                u = v;
        if (u < 0)
            break;
        done[u] = 1;
        for (int v = 0; v < n; v++) {
            if (w[u * n + v] < 0 || done[v])
                continue;
            long long nd = dist[u] + w[u * n + v];
            if (nd < dist[v]) {
                dist[v] = nd;
                ways[v] = ways[u];
            } else if (nd == dist[v]) {
                ways[v] = (ways[v] + ways[u]) % MOD;
            }
        }
    }
    int ans = (int)ways[n - 1];
    free(w);
    free(dist);
    free(ways);
    free(done);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static int run(int n, int e[][3], int m)
{
    int *rows[16], cols[16];
    for (int i = 0; i < m; i++) {
        rows[i] = e[i];
        cols[i] = 3;
    }
    return countPaths(n, rows, m, cols);
}

int main(void)
{
    int a[][3] = {{0, 6, 7}, {0, 1, 2}, {1, 2, 3}, {1, 3, 3}, {6, 3, 3},
                  {3, 5, 1}, {6, 5, 1}, {2, 5, 1}, {0, 4, 5}, {4, 6, 2}};
    assert(run(7, a, 10) == 4);
    int b[][3] = {{1, 0, 10}};
    assert(run(2, b, 1) == 1);
    assert(run(1, NULL, 0) == 1); /* 起點就是終點 */
    /* k 個菱形串起來：每個菱形有上下兩條一樣長的路 → 共 2^k 條最短路徑。
     * 每條邊 10^9，總距離 2k × 10^9 超過 int，順便測 long long。 */
    enum { K = 10 };
    int d[4 * K][3], m = 0;
    for (int i = 0; i < K; i++) {
        int s0 = 3 * i, x = s0 + 1, y = s0 + 2, t = s0 + 3;
        int es[4][2] = {{s0, x}, {x, t}, {s0, y}, {y, t}};
        for (int j = 0; j < 4; j++) {
            d[m][0] = es[j][0];
            d[m][1] = es[j][1];
            d[m++][2] = 1000000000;
        }
    }
    int *rows[4 * K], cols[4 * K];
    for (int i = 0; i < m; i++) {
        rows[i] = d[i];
        cols[i] = 3;
    }
    assert(countPaths(3 * K + 1, rows, m, cols) == 1 << K);
    puts("1976: passed");
    return 0;
}
