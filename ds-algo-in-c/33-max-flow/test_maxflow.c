#include "maxflow.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAXN = 8 };

/* 流量守恆 + 不超過容量 */
static void check_flow(int n, const long long *cap0, const long long *flow, int s, int t, long long value)
{
    for (int i = 0; i < n * n; i++)
        assert(0 <= flow[i] && flow[i] <= cap0[i]);
    for (int v = 0; v < n; v++) {
        long long in = 0, out = 0;
        for (int u = 0; u < n; u++) {
            in += flow[u * n + v];
            out += flow[v * n + u];
        }
        if (v == s)
            assert(out - in == value);
        else if (v == t)
            assert(in - out == value);
        else
            assert(in == out); /* 中間的頂點：流進 = 流出 */
    }
}

static void test_clrs(void)
{
    /* CLRS Figure 24.6：s v1 v2 v3 v4 t = 0..5，最大流 23 */
    long long cap[36] = {0}, cap0[36], flow[36];
    int e[][3] = {{0, 1, 16}, {0, 2, 13}, {2, 1, 4}, {1, 3, 12}, {3, 2, 9}, {2, 4, 14}, {4, 3, 7}, {3, 5, 20}, {4, 5, 4}};
    for (int i = 0; i < 9; i++)
        cap[e[i][0] * 6 + e[i][1]] = e[i][2];
    memcpy(cap0, cap, sizeof cap);
    long long f = edmonds_karp(6, cap, 0, 5, flow);
    assert(f == 23);
    check_flow(6, cap0, flow, 0, 5, f);
    char side[6];
    min_cut_side(6, cap, 0, side);
    long long cut = 0;
    for (int u = 0; u < 6; u++)
        for (int v = 0; v < 6; v++)
            if (side[u] && !side[v])
                cut += cap0[u * 6 + v];
    assert(cut == 23); /* 最大流 = 最小割 */
}

static void test_random(void)
{
    srand(33);
    for (int t = 0; t < 1000; t++) {
        int n = 2 + rand() % (MAXN - 1);
        long long cap[MAXN * MAXN] = {0}, cap0[MAXN * MAXN], flow[MAXN * MAXN];
        for (int u = 0; u < n; u++)
            for (int v = 0; v < n; v++)
                if (u != v && rand() % 3 == 0)
                    cap[u * n + v] = rand() % 10;
        memcpy(cap0, cap, sizeof cap);
        int s = 0, sink = n - 1;
        long long f = edmonds_karp(n, cap, s, sink, flow);
        check_flow(n, cap0, flow, s, sink, f);

        /* 最大流最小割定理：暴力枚舉所有 s-t 割，最小的容量 = 最大流 */
        long long best = -1;
        for (int mask = 0; mask < 1 << n; mask++) {
            if (!(mask & 1) || (mask >> sink & 1))
                continue; /* s 在 S 側、t 在 T 側 */
            long long c = 0;
            for (int u = 0; u < n; u++)
                for (int v = 0; v < n; v++)
                    if ((mask >> u & 1) && !(mask >> v & 1))
                        c += cap0[u * n + v];
            if (best < 0 || c < best)
                best = c;
        }
        assert(f == best);

        char side[MAXN];
        min_cut_side(n, cap, s, side);
        assert(side[s] && !side[sink]);
        long long cut = 0;
        for (int u = 0; u < n; u++)
            for (int v = 0; v < n; v++)
                if (side[u] && !side[v])
                    cut += cap0[u * n + v];
        assert(cut == f);
    }
}

static void test_matching(void)
{
    srand(3333);
    for (int t = 0; t < 1000; t++) {
        int nl = 1 + rand() % 7, nr = 1 + rand() % 7, m = 0, e[49][2], adj[7] = {0};
        for (int u = 0; u < nl; u++)
            for (int v = 0; v < nr; v++)
                if (rand() % 3 == 0) {
                    e[m][0] = u;
                    e[m++][1] = v;
                    adj[u] |= 1 << v;
                }
        int match[7];
        int got = bipartite_matching(nl, nr, e, m, match);
        /* 配對要合法：每條都是真的邊、右邊不重複 */
        int used = 0, cnt = 0;
        for (int u = 0; u < nl; u++)
            if (match[u] >= 0) {
                assert(adj[u] >> match[u] & 1);
                assert(!(used >> match[u] & 1));
                used |= 1 << match[u];
                cnt++;
            }
        assert(cnt == got);
        /* 暴力：枚舉左邊頂點的子集合，用 Hall 定理檢查這些頂點能不能全部配上 */
        int best = 0;
        for (int mask = 0; mask < 1 << nl; mask++) { /* 選哪些左邊頂點要配對，再看能不能全部配上 */
            int k = __builtin_popcount((unsigned)mask);
            if (k <= best)
                continue;
            /* 檢查這些左邊頂點能否完美配對：Hall 條件（每個子集合的鄰居數 >= 子集合大小） */
            int ok = 1;
            for (int sub = mask; sub && ok; sub = (sub - 1) & mask) {
                int nb = 0;
                for (int u = 0; u < nl; u++)
                    if (sub >> u & 1)
                        nb |= adj[u];
                ok = __builtin_popcount((unsigned)nb) >= __builtin_popcount((unsigned)sub);
            }
            if (ok)
                best = k;
        }
        assert(got == best);
    }
}

int main(void)
{
    test_clrs();
    test_random();
    test_matching();
    puts("maxflow: all tests passed");
    return 0;
}
