/* LeetCode 787 · Cheapest Flights Within K Stops
 * 從 src 到 dst，最多轉機 k 次（= 最多搭 k+1 段航班），最便宜多少錢？
 * 思路：Bellman-Ford 只做 k+1 輪。第 i 輪結束後，dist 是「最多用 i 條邊」的最短距離。
 *   關鍵：每一輪都要用「上一輪的 dist」來鬆弛（先複製一份），
 *   否則同一輪裡可能連續用好幾條邊，就超過 k 次轉機的限制了。O(k · E)。
 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
int findCheapestPrice(int n, int **flights, int flightsSize, int *flightsColSize, int src, int dst, int k)
{
    (void)flightsColSize;
    int dist[100], prev[100]; /* 題目：n <= 100 */
    for (int i = 0; i < n; i++)
        dist[i] = INT_MAX;
    dist[src] = 0;
    for (int round = 0; round <= k; round++) {
        memcpy(prev, dist, (size_t)n * sizeof *dist);
        for (int i = 0; i < flightsSize; i++) {
            int u = flights[i][0], v = flights[i][1], w = flights[i][2];
            if (prev[u] != INT_MAX && prev[u] + w < dist[v])
                dist[v] = prev[u] + w; /* 只從「上一輪」延伸一條邊 */
        }
    }
    return dist[dst] == INT_MAX ? -1 : dist[dst];
}
/* ===== 提交範圍 結束 ===== */

static int run(int n, int f[][3], int m, int s, int d, int k)
{
    int *rows[16], cols[16];
    for (int i = 0; i < m; i++) {
        rows[i] = f[i];
        cols[i] = 3;
    }
    return findCheapestPrice(n, rows, m, cols, s, d, k);
}

int main(void)
{
    int a[][3] = {{0, 1, 100}, {1, 2, 100}, {2, 0, 100}, {1, 3, 600}, {2, 3, 200}};
    assert(run(4, a, 5, 0, 3, 1) == 700); /* 0→1→2→3 只要 400，但轉了 2 次 */
    int b[][3] = {{0, 1, 100}, {1, 2, 100}, {0, 2, 500}};
    assert(run(3, b, 3, 0, 2, 1) == 200);
    assert(run(3, b, 3, 0, 2, 0) == 500);
    /* 沒複製 prev 的錯誤寫法，在這個例子會算出 2（0→1→2 在同一輪裡連走兩條邊） */
    int c[][3] = {{0, 1, 1}, {1, 2, 1}, {0, 2, 5}};
    assert(run(3, c, 3, 0, 2, 0) == 5);
    int d[][3] = {{0, 1, 5}};
    assert(run(3, d, 1, 0, 2, 5) == -1);
    puts("0787: passed");
    return 0;
}
