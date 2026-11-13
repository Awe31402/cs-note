#include "bellman_ford.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 暴力：Floyd-Warshall。d[i][i] < 0 代表 i 在負環上 */
static void floyd(int n, const BEdge *e, int m, long long d[][12])
{
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            d[i][j] = i == j ? 0 : BF_INF;
    for (int i = 0; i < m; i++)
        if (e[i].w < d[e[i].u][e[i].v])
            d[e[i].u][e[i].v] = e[i].w;
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                if (d[i][k] < BF_INF && d[k][j] < BF_INF && d[i][k] + d[k][j] < d[i][j])
                    d[i][j] = d[i][k] + d[k][j];
}

static void test_clrs(void)
{
    /* CLRS Figure 22.4：s t x y z = 0..4，有負權重但沒有負環 */
    BEdge e[] = {{0, 1, 6}, {0, 3, 7}, {1, 2, 5}, {1, 3, 8}, {1, 4, -4},
                 {2, 1, -2}, {3, 2, -3}, {3, 4, 9}, {4, 0, 2}, {4, 2, 7}};
    long long d[5];
    int p[5];
    assert(bellman_ford(5, e, 10, 0, d, p));
    long long want[] = {0, 2, 4, 7, -2};
    assert(memcmp(d, want, sizeof want) == 0);
    int cyc[5];
    assert(find_negative_cycle(5, e, 10, cyc) == 0);
}

static void test_random(void)
{
    srand(25);
    int cycles_seen = 0;
    for (int t = 0; t < 3000; t++) {
        int n = 1 + rand() % 10, m = rand() % (2 * n + 1);
        BEdge e[30];
        for (int i = 0; i < m; i++)
            e[i] = (BEdge){rand() % n, rand() % n, rand() % 20 - 5}; /* 有負權重，有時會有負環 */
        long long fw[12][12];
        floyd(n, e, m, fw);
        int any_neg = 0;
        for (int i = 0; i < n; i++)
            any_neg |= fw[i][i] < 0;

        int s = rand() % n, p[12];
        long long d[12];
        int s_reaches_neg = 0; /* s 走得到某個負環上的頂點 */
        for (int v = 0; v < n; v++)
            if (fw[v][v] < 0 && fw[s][v] < BF_INF)
                s_reaches_neg = 1;
        int ok = bellman_ford(n, e, m, s, d, p);
        assert(ok == !s_reaches_neg);
        if (ok)
            for (int v = 0; v < n; v++)
                assert(d[v] == fw[s][v]);

        int cyc[12];
        int k = find_negative_cycle(n, e, m, cyc);
        assert((k > 0) == any_neg);
        if (k > 0) { /* 環上每一步都有邊，而且總和真的是負的 */
            cycles_seen++;
            long long sum = 0;
            for (int i = 0; i < k; i++) {
                int best = 1 << 30;
                for (int j = 0; j < m; j++)
                    if (e[j].u == cyc[i] && e[j].v == cyc[(i + 1) % k] && e[j].w < best)
                        best = e[j].w;
                assert(best != 1 << 30);
                sum += best;
            }
            assert(sum < 0);
        }
    }
    assert(cycles_seen > 100); /* 確認測試真的有涵蓋到負環 */
}

static void test_dag(void)
{
    /* CLRS Figure 22.5：r s t x y z = 0..5，從 s 出發 */
    BEdge e[] = {{0, 1, 5}, {0, 2, 3}, {1, 2, 2}, {1, 3, 6}, {2, 3, 7}, {2, 4, 4},
                 {2, 5, 2}, {3, 4, -1}, {3, 5, 1}, {4, 5, -2}};
    long long d[6];
    int p[6];
    assert(dag_shortest_paths(6, e, 10, 1, d, p));
    long long want[] = {BF_INF, 0, 2, 6, 5, 3};
    assert(memcmp(d, want, sizeof want) == 0);
    BEdge cyc[] = {{0, 1, 1}, {1, 0, 1}};
    assert(!dag_shortest_paths(2, cyc, 2, 0, d, p));
}

int main(void)
{
    test_clrs();
    test_random();
    test_dag();
    puts("bellman_ford: all tests passed");
    return 0;
}
