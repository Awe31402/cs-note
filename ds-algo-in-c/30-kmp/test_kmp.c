#include "kmp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_prefix(void)
{
    /* CLRS Figure 32.10：P = ababaca */
    int pi[7], want[] = {0, 0, 1, 2, 3, 0, 1};
    kmp_prefix("ababaca", 7, pi);
    assert(memcmp(pi, want, sizeof want) == 0);
    int pi2[6], want2[] = {0, 1, 2, 3, 4, 5};
    kmp_prefix("aaaaaa", 6, pi2);
    assert(memcmp(pi2, want2, sizeof want2) == 0);
}

static void test_examples(void)
{
    int a[16], b[16], c[16];
    /* CLRS Figure 32.1：T = abcabaabcabac, P = abaa → 在 shift 3 */
    assert(naive_match("abcabaabcabac", "abaa", a) == 1 && a[0] == 3);
    assert(kmp_match("abcabaabcabac", "abaa", b) == 1 && b[0] == 3);
    assert(rabin_karp("abcabaabcabac", "abaa", c) == 1 && c[0] == 3);
    /* 重疊的出現 */
    assert(kmp_match("aaaaa", "aa", b) == 4);
    assert(rabin_karp("aaaaa", "aa", c) == 4);
    assert(kmp_match("abc", "abcd", b) == 0 && rabin_karp("abc", "abcd", c) == 0);
}

static void random_str(char *s, int n, int alpha)
{
    for (int i = 0; i < n; i++)
        s[i] = (char)('a' + rand() % alpha);
    s[n] = '\0';
}

static void test_random(void)
{
    srand(30);
    for (int t = 0; t < 5000; t++) {
        char T[64], P[10];
        int alpha = 1 + rand() % 3; /* 字母很少 → 大量部分匹配與重疊 */
        random_str(T, rand() % 60, alpha);
        random_str(P, 1 + rand() % 6, alpha);
        int a[64], b[64], c[64];
        int na = naive_match(T, P, a), nb = kmp_match(T, P, b), nc = rabin_karp(T, P, c);
        assert(na == nb && na == nc);
        assert(memcmp(a, b, (size_t)na * sizeof *a) == 0 && memcmp(a, c, (size_t)na * sizeof *a) == 0);

        /* 前綴函數對照定義 */
        int m = (int)strlen(P), pi[10];
        kmp_prefix(P, m, pi);
        for (int q = 0; q < m; q++) {
            int want = 0;
            for (int k = q; k >= 1; k--) /* 最長的 k < q+1，使 P[0..k) == P[q+1-k..q] */
                if (memcmp(P, P + q + 1 - k, (size_t)k) == 0) {
                    want = k;
                    break;
                }
            assert(pi[q] == want);
        }

        /* Z 函數對照定義 */
        int n = (int)strlen(T), z[64];
        z_function(T, n, z);
        for (int i = 1; i < n; i++) {
            int want = 0;
            while (i + want < n && T[want] == T[i + want])
                want++;
            assert(z[i] == want);
        }
    }
}

static void test_long(void)
{
    /* 最壞情況：T = aaaa...ab、P = aaaab。暴力法 O(nm)，KMP O(n + m) */
    enum { N = 200000, M = 1000 };
    char *T = malloc(N + 1), *P = malloc(M + 1);
    memset(T, 'a', N);
    T[N - 1] = 'b';
    T[N] = '\0';
    memset(P, 'a', M);
    P[M - 1] = 'b';
    P[M] = '\0';
    int pos[4];
    assert(kmp_match(T, P, pos) == 1 && pos[0] == N - M);
    assert(rabin_karp(T, P, pos) == 1 && pos[0] == N - M);
    free(T);
    free(P);
}

int main(void)
{
    test_prefix();
    test_examples();
    test_random();
    test_long();
    puts("kmp: all tests passed");
    return 0;
}
