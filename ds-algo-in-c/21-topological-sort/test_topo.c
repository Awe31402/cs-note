#include "topo.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int has_edge(const Digraph *g, int u, int v)
{
    for (int i = g->start[u]; i < g->start[u + 1]; i++)
        if (g->adj[i] == v)
            return 1;
    return 0;
}

/* order 是一個排列，而且每條邊 u → v 都滿足 pos[u] < pos[v] */
static void check_order(const Digraph *g, const int *order)
{
    int n = g->n, pos[40];
    for (int v = 0; v < n; v++)
        pos[v] = -1;
    for (int i = 0; i < n; i++) {
        assert(pos[order[i]] == -1);
        pos[order[i]] = i;
    }
    for (int u = 0; u < n; u++)
        for (int i = g->start[u]; i < g->start[u + 1]; i++)
            assert(pos[u] < pos[g->adj[i]]);
}

/* 暴力：Warshall 遞移閉包，有頂點能走回自己就是有環 */
static int brute_has_cycle(const Digraph *g)
{
    int n = g->n;
    static char r[40][40];
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            r[i][j] = (char)has_edge(g, i, j);
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                if (r[i][k] && r[k][j])
                    r[i][j] = 1;
    for (int i = 0; i < n; i++)
        if (r[i][i])
            return 1;
    return 0;
}

static void test_clrs_example(void)
{
    /* CLRS Figure 20.7：穿衣服。0 內褲 1 褲子 2 皮帶 3 襯衫 4 領帶 5 外套 6 襪子 7 鞋子 8 手錶 */
    int e[][2] = {{0, 1}, {0, 7}, {1, 2}, {1, 7}, {2, 5}, {3, 2}, {3, 4}, {4, 5}, {6, 7}};
    Digraph g;
    digraph_build(&g, 9, e, 9);
    int a[9], b[9];
    assert(topo_kahn(&g, a) == 9);
    check_order(&g, a);
    assert(topo_dfs(&g, b));
    check_order(&g, b);
    assert(find_cycle(&g, a) == 0);
    digraph_free(&g);
}

static void test_random(void)
{
    srand(21);
    for (int t = 0; t < 1000; t++) {
        int n = 1 + rand() % 25, m = rand() % (2 * n);
        int e[60][2];
        int dag = t % 2;
        int perm[40];
        for (int i = 0; i < n; i++)
            perm[i] = i;
        for (int i = n - 1; i > 0; i--) {
            int j = rand() % (i + 1), x = perm[i];
            perm[i] = perm[j];
            perm[j] = x;
        }
        for (int i = 0; i < m; i++) {
            int a = rand() % n, b = rand() % n;
            if (dag) { /* 一定是 DAG：只從排列中前面的指向後面的 */
                if (a == b)
                    b = (b + 1) % n;
                if (a > b) {
                    int x = a;
                    a = b;
                    b = x;
                }
                if (a == b) { /* n == 1 */
                    m = i;
                    break;
                }
                a = perm[a];
                b = perm[b];
            }
            e[i][0] = a;
            e[i][1] = b;
        }
        Digraph g;
        digraph_build(&g, n, e, m);
        int want_cycle = brute_has_cycle(&g);
        if (dag)
            assert(!want_cycle);
        int order[40], cycle[40];
        int k = topo_kahn(&g, order);
        assert((k < n) == want_cycle);
        if (!want_cycle)
            check_order(&g, order);
        assert(topo_dfs(&g, order) == !want_cycle);
        if (!want_cycle)
            check_order(&g, order);
        int len = find_cycle(&g, cycle);
        assert((len > 0) == want_cycle);
        for (int i = 0; i < len; i++) /* 環上每一步都是真的邊 */
            assert(has_edge(&g, cycle[i], cycle[(i + 1) % len]));
        digraph_free(&g);
    }
}

int main(void)
{
    test_clrs_example();
    test_random();
    puts("topo: all tests passed");
    return 0;
}
