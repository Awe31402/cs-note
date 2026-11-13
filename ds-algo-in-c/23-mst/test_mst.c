#include "mst.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* chosen 剛好 n-1 條、全部是圖上的邊、連起所有頂點、權重加總正確 */
static void check_tree(int n, const Edge *all, int m, const Edge *chosen, int k, long long total)
{
    assert(k == n - 1);
    int p[64];
    for (int i = 0; i < n; i++)
        p[i] = i;
    long long sum = 0;
    for (int i = 0; i < k; i++) {
        int ok = 0;
        for (int j = 0; j < m && !ok; j++)
            ok = all[j].w == chosen[i].w && ((all[j].u == chosen[i].u && all[j].v == chosen[i].v) ||
                                             (all[j].u == chosen[i].v && all[j].v == chosen[i].u));
        assert(ok);
        int a = chosen[i].u, b = chosen[i].v;
        while (p[a] != a)
            a = p[a];
        while (p[b] != b)
            b = p[b];
        assert(a != b); /* 沒有環 */
        p[a] = b;
        sum += chosen[i].w;
    }
    assert(sum == total);
}

/* 暴力：試遍所有 n-1 條邊的組合，挑出能連通的最小總和（只適用很小的圖） */
static long long brute(int n, const Edge *e, int m)
{
    long long best = -1;
    for (int mask = 0; mask < (1 << m); mask++) {
        if (__builtin_popcount((unsigned)mask) != n - 1)
            continue;
        int p[8], ok = 1;
        long long s = 0;
        for (int i = 0; i < n; i++)
            p[i] = i;
        for (int i = 0; i < m && ok; i++)
            if (mask >> i & 1) {
                int a = e[i].u, b = e[i].v;
                while (p[a] != a)
                    a = p[a];
                while (p[b] != b)
                    b = p[b];
                ok = a != b;
                p[a] = b;
                s += e[i].w;
            }
        if (ok && (best < 0 || s < best))
            best = s;
    }
    return best;
}

static void test_clrs(void)
{
    /* CLRS Figure 21.1：a..i → 0..8，MST 總權重 37 */
    Edge e[] = {{0, 1, 4}, {0, 7, 8}, {1, 2, 8}, {1, 7, 11}, {2, 3, 7}, {2, 8, 2}, {2, 5, 4},
                {3, 4, 9}, {3, 5, 14}, {4, 5, 10}, {5, 6, 2}, {6, 8, 6}, {6, 7, 1}, {7, 8, 7}};
    int m = 14, k;
    Edge copy[14], chosen[8];
    memcpy(copy, e, sizeof e);
    long long a = kruskal(9, copy, m, chosen, &k);
    assert(a == 37);
    check_tree(9, e, m, chosen, k, a);
    assert(prim_heap(9, e, m, chosen, &k) == 37);
    check_tree(9, e, m, chosen, k, 37);
    int w[81], parent[9];
    memset(w, -1, sizeof w);
    for (int i = 0; i < m; i++)
        w[e[i].u * 9 + e[i].v] = w[e[i].v * 9 + e[i].u] = e[i].w;
    assert(prim_dense(9, w, parent) == 37);
}

static void test_random(void)
{
    srand(23);
    for (int t = 0; t < 2000; t++) {
        int n = 1 + rand() % 7, m = 0;
        Edge e[40], copy[40], chosen[64];
        for (int v = 1; v < n; v++) /* 先隨機接成一棵樹，保證連通 */
            e[m++] = (Edge){rand() % v, v, rand() % 10};
        int extra = rand() % 6;
        for (int i = 0; i < extra && n > 1 && m < 14; i++) {
            int a = rand() % n, b = rand() % n;
            if (a != b)
                e[m++] = (Edge){a, b, rand() % 10};
        }
        long long want = n == 1 ? 0 : brute(n, e, m);
        int k;
        memcpy(copy, e, sizeof e);
        long long a = kruskal(n, copy, m, chosen, &k);
        assert(a == want);
        check_tree(n, e, m, chosen, k, a);
        long long b = prim_heap(n, e, m, chosen, &k);
        assert(b == want);
        check_tree(n, e, m, chosen, k, b);
        int w[49], parent[7];
        memset(w, -1, sizeof w);
        for (int i = 0; i < m; i++) { /* 重邊取最小 */
            int *c = &w[e[i].u * n + e[i].v];
            if (*c < 0 || e[i].w < *c)
                *c = w[e[i].v * n + e[i].u] = e[i].w;
        }
        assert(prim_dense(n, w, parent) == want);
    }
}

static void test_disconnected(void)
{
    Edge e[] = {{0, 1, 5}, {2, 3, 1}};
    Edge copy[2], chosen[4];
    int k;
    memcpy(copy, e, sizeof e);
    assert(kruskal(4, copy, 2, chosen, &k) == -1);
    assert(prim_heap(4, e, 2, chosen, &k) == -1);
}

int main(void)
{
    test_clrs();
    test_random();
    test_disconnected();
    puts("mst: all tests passed");
    return 0;
}
