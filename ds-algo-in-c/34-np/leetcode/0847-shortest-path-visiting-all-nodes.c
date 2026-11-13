/* LeetCode 847 · Shortest Path Visiting All Nodes（n <= 12）
 * 無向、連通、邊權重 1。從任何點出發、可以重複走，拜訪過所有點的最短路徑長？
 * 這跟漢彌爾頓路徑 (Hamiltonian path) / 旅行推銷員 (TSP) 是同一類 NP-hard 問題。
 * 思路：狀態 = (目前在哪個點, 已經拜訪過的點集合 mask)，共 n · 2^n 個狀態。
 *   所有點同時當起點做 BFS（多源 BFS），第一次走到 mask = 全部 的狀態就是答案。O(2^n · n²)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
int shortestPathLength(int **graph, int graphSize, int *graphColSize)
{
    int n = graphSize, full = (1 << n) - 1;
    if (n == 1)
        return 0;
    int states = n << n; /* 狀態編號 = node · 2^n + mask */
    int *dist = malloc((size_t)states * sizeof *dist), *q = malloc((size_t)states * sizeof *q);
    for (int i = 0; i < states; i++)
        dist[i] = -1;
    int head = 0, tail = 0;
    for (int v = 0; v < n; v++) {
        int s = (v << n) | (1 << v);
        dist[s] = 0;
        q[tail++] = s;
    }
    int ans = -1;
    while (head < tail && ans < 0) {
        int s = q[head++], u = s >> n, mask = s & full;
        for (int i = 0; i < graphColSize[u] && ans < 0; i++) {
            int v = graph[u][i], nm = mask | 1 << v, ns = (v << n) | nm;
            if (dist[ns] >= 0)
                continue;
            dist[ns] = dist[s] + 1;
            if (nm == full)
                ans = dist[ns];
            q[tail++] = ns;
        }
    }
    free(dist);
    free(q);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

/* 對照：先用 Floyd 算所有點對距離，再用 Held-Karp DP（TSP 路徑版）：
 * best[mask][v] = 拜訪過 mask、最後停在 v 的最短長度 */
static int held_karp(int n, int adj[12][12])
{
    int d[12][12];
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            d[i][j] = i == j ? 0 : adj[i][j] ? 1 : 1000;
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                if (d[i][k] + d[k][j] < d[i][j])
                    d[i][j] = d[i][k] + d[k][j];
    static int best[1 << 8][8];
    for (int m = 0; m < 1 << n; m++)
        for (int v = 0; v < n; v++)
            best[m][v] = m == 1 << v ? 0 : 1000;
    for (int m = 1; m < 1 << n; m++)
        for (int v = 0; v < n; v++)
            if ((m >> v & 1) && best[m][v] < 1000)
                for (int w = 0; w < n; w++)
                    if (!(m >> w & 1) && best[m][v] + d[v][w] < best[m | 1 << w][w])
                        best[m | 1 << w][w] = best[m][v] + d[v][w];
    int ans = 1000;
    for (int v = 0; v < n; v++)
        if (best[(1 << n) - 1][v] < ans)
            ans = best[(1 << n) - 1][v];
    return ans;
}

int main(void)
{
    int a0[] = {1, 2, 3}, a1[] = {0}, a2[] = {0}, a3[] = {0}; /* 星形 */
    int *a[] = {a0, a1, a2, a3}, ac[] = {3, 1, 1, 1};
    assert(shortestPathLength(a, 4, ac) == 4);
    int b0[] = {1}, b1[] = {0, 2, 4}, b2[] = {1, 3, 4}, b3[] = {2}, b4[] = {1, 2};
    int *b[] = {b0, b1, b2, b3, b4}, bc[] = {1, 3, 3, 1, 2};
    assert(shortestPathLength(b, 5, bc) == 4);

    srand(847);
    for (int t = 0; t < 300; t++) {
        int n = 1 + rand() % 8, adj[12][12] = {{0}};
        for (int v = 1; v < n; v++) { /* 先接成一棵樹：保證連通 */
            int u = rand() % v;
            adj[u][v] = adj[v][u] = 1;
        }
        for (int e = 0; e < n; e++) {
            int u = rand() % n, v = rand() % n;
            if (u != v)
                adj[u][v] = adj[v][u] = 1;
        }
        int buf[12][12], *g[12], deg[12];
        for (int u = 0; u < n; u++) {
            deg[u] = 0;
            for (int v = 0; v < n; v++)
                if (adj[u][v])
                    buf[u][deg[u]++] = v;
            g[u] = buf[u];
        }
        assert(shortestPathLength(g, n, deg) == held_karp(n, adj));
    }
    puts("0847: passed");
    return 0;
}
