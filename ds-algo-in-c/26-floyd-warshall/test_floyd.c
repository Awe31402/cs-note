#include "floyd.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAXN = 12 };

/* 暴力：從每個起點各做一次 Bellman-Ford */
static void brute(int n, const long long *w, long long *d)
{
    for (int s = 0; s < n; s++) {
        long long *ds = d + s * n;
        for (int v = 0; v < n; v++)
            ds[v] = v == s ? 0 : FW_INF;
        for (int r = 0; r < n; r++)
            for (int u = 0; u < n; u++)
                for (int v = 0; v < n; v++)
                    if (ds[u] < FW_INF && w[u * n + v] < FW_INF && ds[u] + w[u * n + v] < ds[v])
                        ds[v] = ds[u] + w[u * n + v];
    }
}

static void test_clrs(void)
{
    /* CLRS Figure 23.4 的圖（1..5 → 0..4） */
    long long I = FW_INF;
    long long d[25] = {0, 3, 8, I, -4,
                       I, 0, I, 1, 7,
                       I, 4, 0, I, I,
                       2, I, -5, 0, I,
                       I, I, I, 6, 0};
    long long want[25] = {0, 1, -3, 2, -4,
                          3, 0, -4, 1, -1,
                          7, 4, 0, 5, 3,
                          2, -1, -5, 0, -2,
                          8, 5, 1, 6, 0};
    int next[25], path[5];
    floyd_warshall(5, d, next);
    assert(memcmp(d, want, sizeof want) == 0);
    assert(!fw_has_negative_cycle(5, d));
    int len = fw_path(5, next, 0, 1, path); /* 1 → 5 → 4 → 3 → 2 */
    int wp[] = {0, 4, 3, 2, 1};
    assert(len == 5 && memcmp(path, wp, sizeof wp) == 0);
}

static void test_random(void)
{
    srand(26);
    for (int t = 0; t < 1000; t++) {
        int n = 1 + rand() % MAXN;
        long long w[MAXN * MAXN], d[MAXN * MAXN], want[MAXN * MAXN];
        int pot[MAXN], next[MAXN * MAXN], path[MAXN];
        for (int i = 0; i < n; i++)
            pot[i] = rand() % 30;
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) {
                w[i * n + j] = FW_INF;
                if (i != j && rand() % 3 == 0)
                    /* 權重 = 非負的基底 + pot[i] - pot[j]：可以是負的，但任何環的總和 = 基底總和 >= 0，
                     * 所以保證沒有負環（這就是 Johnson 演算法 reweighting 的反向操作） */
                    w[i * n + j] = rand() % 10 + pot[i] - pot[j];
            }
        for (int i = 0; i < n; i++)
            w[i * n + i] = 0;
        memcpy(d, w, sizeof(long long) * (size_t)(n * n));
        floyd_warshall(n, d, next);
        brute(n, w, want);
        assert(memcmp(d, want, sizeof(long long) * (size_t)(n * n)) == 0);
        assert(!fw_has_negative_cycle(n, d));
        for (int u = 0; u < n; u++)
            for (int v = 0; v < n; v++) {
                int len = fw_path(n, next, u, v, path);
                if (d[u * n + v] == FW_INF) {
                    assert(len == 0);
                    continue;
                }
                long long sum = 0;
                for (int i = 0; i + 1 < len; i++)
                    sum += w[path[i] * n + path[i + 1]];
                assert(path[0] == u && path[len - 1] == v && sum == d[u * n + v]);
            }

        /* 遞移閉包 = 距離不是 ∞ */
        unsigned char r[MAXN * MAXN];
        for (int i = 0; i < n * n; i++)
            r[i] = w[i] < FW_INF;
        transitive_closure(n, r);
        for (int i = 0; i < n * n; i++)
            assert(r[i] == (d[i] < FW_INF));
    }
}

static void test_negative_cycle(void)
{
    long long I = FW_INF;
    long long d[9] = {0, 1, I,
                      I, 0, -3,
                      1, I, 0}; /* 0→1→2→0 = 1 - 3 + 1 = -1 */
    floyd_warshall(3, d, NULL);
    assert(fw_has_negative_cycle(3, d));
}

int main(void)
{
    test_clrs();
    test_random();
    test_negative_cycle();
    puts("floyd: all tests passed");
    return 0;
}
