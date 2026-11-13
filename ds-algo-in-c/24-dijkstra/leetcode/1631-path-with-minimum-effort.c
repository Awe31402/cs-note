/* LeetCode 1631 · Path With Minimum Effort
 * 網格上從左上走到右下。一條路徑的「費力程度」= 路上相鄰兩格高度差的「最大值」。求最小的費力程度。
 * 思路：Dijkstra 的變形：路徑的代價不是「相加」而是「取最大」。
 *   max 一樣只會越走越大（不會變小），所以 Dijkstra 的貪心還是成立。O(mn log(mn))。
 *   另解：二分搜尋答案 + BFS，或把邊排序後用 Union-Find（Kruskal 風格）。
 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    int d, v;
} Item;

static void push(Item *h, int *n, Item x)
{
    int i = (*n)++;
    h[i] = x;
    while (i > 0 && h[(i - 1) / 2].d > h[i].d) {
        Item t = h[i];
        h[i] = h[(i - 1) / 2];
        h[(i - 1) / 2] = t;
        i = (i - 1) / 2;
    }
}

static Item pop(Item *h, int *n)
{
    Item top = h[0];
    h[0] = h[--*n];
    for (int i = 0;;) {
        int l = 2 * i + 1, r = l + 1, s = i;
        if (l < *n && h[l].d < h[s].d)
            s = l;
        if (r < *n && h[r].d < h[s].d)
            s = r;
        if (s == i)
            break;
        Item t = h[i];
        h[i] = h[s];
        h[s] = t;
        i = s;
    }
    return top;
}

int minimumEffortPath(int **heights, int heightsSize, int *heightsColSize)
{
    int m = heightsSize, n = heightsColSize[0], N = m * n;
    int *dist = malloc((size_t)N * sizeof *dist);
    Item *h = malloc((size_t)(4 * N + 1) * sizeof *h); /* 每格最多被 4 個鄰居推進來 */
    for (int i = 0; i < N; i++)
        dist[i] = INT_MAX;
    int hs = 0;
    dist[0] = 0;
    push(h, &hs, (Item){0, 0});
    const int dr[] = {1, -1, 0, 0}, dc[] = {0, 0, 1, -1};
    int ans = 0;
    while (hs > 0) {
        Item it = pop(h, &hs);
        if (it.d > dist[it.v])
            continue;
        if (it.v == N - 1) {
            ans = it.d;
            break;
        }
        int r = it.v / n, c = it.v % n;
        for (int k = 0; k < 4; k++) {
            int nr = r + dr[k], nc = c + dc[k];
            if (nr < 0 || nr >= m || nc < 0 || nc >= n)
                continue;
            int step = abs(heights[nr][nc] - heights[r][c]);
            int nd = it.d > step ? it.d : step; /* 取最大，不是相加 */
            if (nd < dist[nr * n + nc]) {
                dist[nr * n + nc] = nd;
                push(h, &hs, (Item){nd, nr * n + nc});
            }
        }
    }
    free(dist);
    free(h);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static int run(int g[][5], int m, int n)
{
    int *rows[5], cols[5];
    for (int i = 0; i < m; i++) {
        rows[i] = g[i];
        cols[i] = n;
    }
    return minimumEffortPath(rows, m, cols);
}

int main(void)
{
    int a[][5] = {{1, 2, 2}, {3, 8, 2}, {5, 3, 5}};
    assert(run(a, 3, 3) == 2);
    int b[][5] = {{1, 2, 3}, {3, 8, 4}, {5, 3, 5}};
    assert(run(b, 3, 3) == 1);
    int c[][5] = {{1, 2, 1, 1, 1}, {1, 2, 1, 2, 1}, {1, 2, 1, 2, 1}, {1, 2, 1, 2, 1}, {1, 1, 1, 2, 1}};
    assert(run(c, 5, 5) == 0); /* 繞一大圈就能完全不爬坡 */
    int d[][5] = {{7}};
    assert(run(d, 1, 1) == 0);
    puts("1631: passed");
    return 0;
}
