#include "greedy.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_activity(void)
{
    /* CLRS Figure 15.1：答案 {a1, a4, a8, a11}（0-based：0 3 7 10） */
    int s[] = {1, 3, 0, 5, 3, 5, 6, 7, 8, 2, 12}, f[] = {4, 5, 6, 7, 9, 9, 10, 11, 12, 14, 16};
    int ch[11];
    assert(activity_select(s, f, 11, ch) == 4);
    assert(ch[0] == 0 && ch[1] == 3 && ch[2] == 7 && ch[3] == 10);

    srand(29);
    for (int t = 0; t < 500; t++) { /* 對照：枚舉所有子集合，找最大的相容集合 */
        int n = 1 + rand() % 12, ss[12], ff[12];
        for (int i = 0; i < n; i++) {
            ss[i] = rand() % 20;
            ff[i] = ss[i] + 1 + rand() % 6;
        }
        for (int i = 1; i < n; i++) /* 依結束時間排序 */
            for (int j = i; j > 0 && ff[j - 1] > ff[j]; j--) {
                int a = ff[j], b = ss[j];
                ff[j] = ff[j - 1];
                ss[j] = ss[j - 1];
                ff[j - 1] = a;
                ss[j - 1] = b;
            }
        int best = 0;
        for (int mask = 0; mask < 1 << n; mask++) {
            int ok = 1, cnt = 0;
            for (int i = 0; i < n && ok; i++)
                for (int j = i + 1; j < n && ok; j++)
                    if ((mask >> i & 1) && (mask >> j & 1) && ss[i] < ff[j] && ss[j] < ff[i])
                        ok = 0;
            for (int i = 0; i < n; i++)
                cnt += mask >> i & 1;
            if (ok && cnt > best)
                best = cnt;
        }
        assert(activity_select(ss, ff, n, ch) == best);
    }
}

/* 另一種獨立的 Huffman：陣列上 O(n²) 找兩個最小的，只算總成本 */
static long long huffman_slow(const long long *freq, int n)
{
    long long w[64], cost = 0;
    memcpy(w, freq, (size_t)n * sizeof *w);
    for (int m = n; m > 1; m--) {
        int a = 0;
        for (int i = 1; i < m; i++)
            if (w[i] < w[a])
                a = i;
        long long x = w[a];
        w[a] = w[m - 1];
        int b = 0;
        for (int i = 1; i < m - 1; i++)
            if (w[i] < w[b])
                b = i;
        long long y = w[b];
        w[b] = x + y;
        cost += x + y;
    }
    return cost;
}

static void check_prefix_free(char (*codes)[64], int n, const long long *freq, long long cost)
{
    long long sum = 0;
    double kraft = 0;
    for (int i = 0; i < n; i++) {
        size_t li = strlen(codes[i]);
        sum += freq[i] * (long long)li;
        kraft += ldexp(1.0, -(int)li);
        for (int j = 0; j < n; j++) /* 沒有任何編碼是別人的前綴 */
            if (i != j)
                assert(strncmp(codes[i], codes[j], li) != 0 || strlen(codes[j]) < li);
    }
    assert(sum == cost);
    assert(fabs(kraft - 1.0) < 1e-12); /* 完滿二元樹：Kraft 等式 Σ 2^(-len) = 1 */
}

static void test_huffman(void)
{
    /* CLRS Figure 15.6：a..f 頻率（千次）45 13 12 16 9 5 → 總共 224 千位元 */
    long long freq[] = {45, 13, 12, 16, 9, 5};
    char codes[6][64];
    long long cost = huffman(freq, 6, codes);
    assert(cost == 224);
    assert(strlen(codes[0]) == 1); /* 最常見的 a 只要 1 位 */
    check_prefix_free(codes, 6, freq, cost);

    srand(290);
    for (int t = 0; t < 500; t++) {
        int n = 2 + rand() % 30;
        long long f[32];
        char c[32][64];
        for (int i = 0; i < n; i++)
            f[i] = 1 + rand() % 100;
        long long got = huffman(f, n, c);
        assert(got == huffman_slow(f, n));
        check_prefix_free(c, n, f, got);
    }
}

static void test_knapsack(void)
{
    /* CLRS 15.2 的例子：容量 50，(10, 60) (20, 100) (30, 120) → 60 + 100 + 120 × 20/30 = 240 */
    int w[] = {10, 20, 30}, v[] = {60, 100, 120};
    assert(fabs(fractional_knapsack(w, v, 3, 50) - 240.0) < 1e-9);
    assert(fabs(fractional_knapsack(w, v, 3, 100) - 280.0) < 1e-9);
}

/* 暴力：DP 走遍所有可能的快取內容（頁面 0..3、容量 k），找最少 miss */
static int cache_brute(const int *req, int m, int k)
{
    enum { P = 4, S = 1 << P };
    int INF = 1 << 20, dp[S], nd[S];
    for (int s = 0; s < S; s++)
        dp[s] = INF;
    dp[0] = 0;
    for (int t = 0; t < m; t++) {
        int p = 1 << req[t];
        for (int s = 0; s < S; s++)
            nd[s] = INF;
        for (int s = 0; s < S; s++) {
            if (dp[s] >= INF)
                continue;
            if (s & p) { /* hit */
                if (dp[s] < nd[s])
                    nd[s] = dp[s];
                continue;
            }
            if (__builtin_popcount((unsigned)s) < k && dp[s] + 1 < nd[s | p])
                nd[s | p] = dp[s] + 1;
            for (int q = 0; q < P; q++) /* 滿了（或故意）踢掉任一頁 */
                if ((s >> q & 1) && dp[s] + 1 < nd[(s & ~(1 << q)) | p])
                    nd[(s & ~(1 << q)) | p] = dp[s] + 1;
        }
        memcpy(dp, nd, sizeof dp);
    }
    int best = INF;
    for (int s = 0; s < S; s++)
        if (dp[s] < best)
            best = dp[s];
    return best;
}

static void test_cache(void)
{
    srand(2929);
    for (int t = 0; t < 1000; t++) {
        int m = 1 + rand() % 14, k = 1 + rand() % 3, req[14];
        for (int i = 0; i < m; i++)
            req[i] = rand() % 4;
        assert(offline_cache_misses(req, m, k) == cache_brute(req, m, k));
    }
}

int main(void)
{
    test_activity();
    test_huffman();
    test_knapsack();
    test_cache();
    puts("greedy: all tests passed");
    return 0;
}
