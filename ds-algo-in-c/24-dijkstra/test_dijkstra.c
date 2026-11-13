#include "dijkstra.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 暴力：Bellman-Ford，做 n-1 輪鬆弛 */
static void brute(int n, const int (*e)[3], int m, int s, long long *d)
{
    for (int v = 0; v < n; v++)
        d[v] = DIST_INF;
    d[s] = 0;
    for (int r = 0; r < n - 1; r++)
        for (int i = 0; i < m; i++)
            if (d[e[i][0]] < DIST_INF && d[e[i][0]] + e[i][2] < d[e[i][1]])
                d[e[i][1]] = d[e[i][0]] + e[i][2];
}

static int edge_w(const int (*e)[3], int m, int u, int v) /* u→v 最輕的那條 */
{
    int best = -1;
    for (int i = 0; i < m; i++)
        if (e[i][0] == u && e[i][1] == v && (best < 0 || e[i][2] < best))
            best = e[i][2];
    return best;
}

static void test_clrs(void)
{
    /* CLRS Figure 22.6：s t x y z → 0..4 */
    int e[][3] = {{0, 1, 10}, {0, 3, 5}, {1, 2, 1}, {1, 3, 2}, {2, 4, 4},
                  {3, 1, 3}, {3, 2, 9}, {3, 4, 2}, {4, 0, 7}, {4, 2, 6}};
    WGraph g;
    wgraph_build(&g, 5, e, 10);
    long long d[5];
    int p[5], path[5];
    dijkstra(&g, 0, d, p);
    long long want[] = {0, 8, 9, 5, 7};
    assert(memcmp(d, want, sizeof want) == 0);
    int len = dij_path(p, 0, 2, path); /* s → y → t → x */
    assert(len == 4 && path[1] == 3 && path[2] == 1 && path[3] == 2);
    wgraph_free(&g);
}

static void test_random(void)
{
    srand(24);
    for (int t = 0; t < 1000; t++) {
        int n = 1 + rand() % 15, m = rand() % (3 * n + 1);
        int e[50][3], w[225];
        memset(w, -1, sizeof w);
        for (int i = 0; i < m; i++) {
            e[i][0] = rand() % n;
            e[i][1] = rand() % n;
            e[i][2] = rand() % 20; /* 可能有 0 權重的邊 */
            int *c = &w[e[i][0] * n + e[i][1]];
            if (*c < 0 || e[i][2] < *c)
                *c = e[i][2];
        }
        WGraph g;
        wgraph_build(&g, n, e, m);
        int s = rand() % n, p[15], p2[15], path[15];
        long long d[15], d2[15], want[15];
        dijkstra(&g, s, d, p);
        dijkstra_dense(n, w, s, d2, p2);
        brute(n, e, m, s, want);
        assert(memcmp(d, want, (size_t)n * sizeof *d) == 0);
        assert(memcmp(d2, want, (size_t)n * sizeof *d2) == 0);
        for (int v = 0; v < n; v++) {
            if (d[v] == DIST_INF) {
                assert(dij_path(p, s, v, path) == 0 || v == s);
                continue;
            }
            int len = dij_path(p, s, v, path);
            long long sum = 0;
            for (int i = 0; i + 1 < len; i++) { /* 路徑上每一步都是真的邊，加起來剛好等於距離 */
                int c = edge_w(e, m, path[i], path[i + 1]);
                assert(c >= 0);
                sum += c;
            }
            assert(sum == d[v]);
        }
        wgraph_free(&g);
    }
}

int main(void)
{
    test_clrs();
    test_random();
    puts("dijkstra: all tests passed");
    return 0;
}
