#include "graph.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_example(void)
{
    /* CLRS Figure 20.3 的無向圖：r s t u v w x y → 0..7 */
    enum { R, S, T, U, V, W, X, Y };
    Graph g;
    graph_init(&g, 8);
    int e[][2] = {{R, S}, {R, V}, {S, W}, {W, T}, {W, X}, {T, X}, {T, U}, {X, U}, {X, Y}, {U, Y}};
    for (int i = 0; i < 10; i++)
        graph_add_undirected(&g, e[i][0], e[i][1]);
    int dist[8], parent[8], path[8];
    assert(bfs(&g, S, dist, parent) == 8);
    int want[8] = {1, 0, 2, 3, 2, 1, 2, 3};
    assert(memcmp(dist, want, sizeof want) == 0);
    int len = build_path(parent, S, Y, path);
    assert(len == 4 && path[0] == S && path[3] == Y);
    graph_free(&g);
}

/* 暴力算最短邊數：反覆鬆弛直到不變（每條邊權重 1 的 Bellman-Ford） */
static void brute_dist(const Graph *g, int s, int *dist)
{
    for (int v = 0; v < g->n; v++)
        dist[v] = -1;
    dist[s] = 0;
    for (int round = 0; round < g->n; round++)
        for (int u = 0; u < g->n; u++)
            if (dist[u] >= 0)
                for (int i = 0; i < g->deg[u]; i++) {
                    int v = g->adj[u][i];
                    if (dist[v] == -1 || dist[u] + 1 < dist[v])
                        dist[v] = dist[u] + 1;
                }
}

static void test_random(void)
{
    srand(20);
    for (int t = 0; t < 300; t++) {
        int n = 1 + rand() % 30, m = rand() % (2 * n + 1);
        Graph g;
        graph_init(&g, n);
        for (int i = 0; i < m; i++)
            graph_add_edge(&g, rand() % n, rand() % n);

        int dist[30], parent[30], want[30], path[30];
        int s = rand() % n;
        int reached = bfs(&g, s, dist, parent);
        brute_dist(&g, s, want);
        assert(memcmp(dist, want, (size_t)n * sizeof *dist) == 0);
        int cnt = 0;
        for (int v = 0; v < n; v++) {
            cnt += dist[v] >= 0;
            if (dist[v] >= 0) /* 路徑長度 = 距離 + 1，而且每一步都是真的邊 */
                assert(build_path(parent, s, v, path) == dist[v] + 1);
        }
        assert(cnt == reached);

        /* DFS 的括號定理 (parenthesis theorem)：任兩個頂點的 [d, f] 區間，不是不相交就是一個包含另一個 */
        int d[30], f[30], p[30];
        dfs(&g, d, f, p);
        for (int u = 0; u < n; u++) {
            assert(1 <= d[u] && d[u] < f[u] && f[u] <= 2 * n);
            for (int v = 0; v < n; v++) {
                if (u == v)
                    continue;
                int disjoint = f[u] < d[v] || f[v] < d[u];
                int u_in_v = d[v] < d[u] && f[u] < f[v];
                int v_in_u = d[u] < d[v] && f[v] < f[u];
                assert(disjoint + u_in_v + v_in_u == 1);
            }
            if (p[u] >= 0) /* 樹邊：小孩的區間在父節點裡面 */
                assert(d[p[u]] < d[u] && f[u] < f[p[u]]);
        }

        /* 迴圈版 DFS 走到的頂點 = BFS 走到的頂點 */
        int order[30];
        int k = dfs_iter(&g, s, order);
        assert(k == reached && order[0] == s);
        for (int i = 0; i < k; i++)
            assert(dist[order[i]] >= 0);
        graph_free(&g);
    }
}

static void test_components(void)
{
    Graph g;
    graph_init(&g, 7);
    graph_add_undirected(&g, 0, 1);
    graph_add_undirected(&g, 1, 2);
    graph_add_undirected(&g, 3, 4);
    int comp[7];
    assert(connected_components(&g, comp) == 4); /* {0,1,2} {3,4} {5} {6} */
    assert(comp[0] == comp[2] && comp[3] == comp[4] && comp[0] != comp[3] && comp[5] != comp[6]);
    graph_free(&g);
}

int main(void)
{
    test_example();
    test_random();
    test_components();
    puts("graph: all tests passed");
    return 0;
}
