/* LeetCode 1334 · Find the City With the Smallest Number of Neighbors at a Threshold Distance
 * 對每個城市，數「距離 <= threshold」的其他城市有幾個；找最少的那個城市（一樣多取編號大的）。
 * 思路：要所有點對的距離 → Floyd-Warshall，n <= 100，O(n³) = 10^6。
 */
#include <assert.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
int findTheCity(int n, int **edges, int edgesSize, int *edgesColSize, int distanceThreshold)
{
    (void)edgesColSize;
    const int INF = 1 << 29; /* 兩個 INF 相加也不會溢位 */
    int d[100][100];
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            d[i][j] = i == j ? 0 : INF;
    for (int i = 0; i < edgesSize; i++) { /* 無向圖 */
        int a = edges[i][0], b = edges[i][1], w = edges[i][2];
        d[a][b] = d[b][a] = w;
    }
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                if (d[i][k] + d[k][j] < d[i][j])
                    d[i][j] = d[i][k] + d[k][j];
    int best = -1, best_cnt = n + 1;
    for (int i = 0; i < n; i++) {
        int cnt = 0;
        for (int j = 0; j < n; j++)
            cnt += i != j && d[i][j] <= distanceThreshold;
        if (cnt <= best_cnt) { /* <=：一樣多時，後面（編號大）的覆蓋前面的 */
            best_cnt = cnt;
            best = i;
        }
    }
    return best;
}
/* ===== 提交範圍 結束 ===== */

static int run(int n, int e[][3], int m, int th)
{
    int *rows[16], cols[16];
    for (int i = 0; i < m; i++) {
        rows[i] = e[i];
        cols[i] = 3;
    }
    return findTheCity(n, rows, m, cols, th);
}

int main(void)
{
    int a[][3] = {{0, 1, 3}, {1, 2, 1}, {1, 3, 4}, {2, 3, 1}};
    assert(run(4, a, 4, 4) == 3);
    int b[][3] = {{0, 1, 2}, {0, 4, 8}, {1, 2, 3}, {1, 4, 2}, {2, 3, 1}, {3, 4, 1}};
    assert(run(5, b, 6, 2) == 0);
    puts("1334: passed");
    return 0;
}
