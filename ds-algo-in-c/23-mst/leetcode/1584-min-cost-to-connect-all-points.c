/* LeetCode 1584 · Min Cost to Connect All Points
 * 點之間的距離是曼哈頓距離 |x1-x2| + |y1-y2|，任兩點都能連 → 完全圖，E = V²。
 * 思路：稠密圖用陣列版 Prim，O(V²)，不用真的把 V² 條邊存下來。
 *       Kruskal 要排序 V² 條邊，O(V² log V)，比較慢也比較耗記憶體。
 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int minCostConnectPoints(int **points, int pointsSize, int *pointsColSize)
{
    (void)pointsColSize;
    int n = pointsSize, total = 0;
    int *key = malloc((size_t)n * sizeof *key);
    char *in_tree = calloc((size_t)n, 1);
    for (int i = 0; i < n; i++)
        key[i] = INT_MAX;
    key[0] = 0;
    for (int round = 0; round < n; round++) {
        int u = -1;
        for (int v = 0; v < n; v++)
            if (!in_tree[v] && (u < 0 || key[v] < key[u]))
                u = v;
        in_tree[u] = 1;
        total += key[u];
        for (int v = 0; v < n; v++) {
            if (in_tree[v])
                continue;
            int d = abs(points[u][0] - points[v][0]) + abs(points[u][1] - points[v][1]);
            if (d < key[v])
                key[v] = d;
        }
    }
    free(key);
    free(in_tree);
    return total;
}
/* ===== 提交範圍 結束 ===== */

static int run(int p[][2], int n)
{
    int *rows[8], cols[8];
    for (int i = 0; i < n; i++) {
        rows[i] = p[i];
        cols[i] = 2;
    }
    return minCostConnectPoints(rows, n, cols);
}

int main(void)
{
    int a[][2] = {{0, 0}, {2, 2}, {3, 10}, {5, 2}, {7, 0}};
    assert(run(a, 5) == 20);
    int b[][2] = {{3, 12}, {-2, 5}, {-4, 1}};
    assert(run(b, 3) == 18);
    int c[][2] = {{0, 0}};
    assert(run(c, 1) == 0);
    int d[][2] = {{-1000000, -1000000}, {1000000, 1000000}};
    assert(run(d, 2) == 4000000);
    puts("1584: passed");
    return 0;
}
