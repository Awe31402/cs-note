#include "dp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_rod(void)
{
    /* CLRS Figure 14.1 的價目表 */
    int price[] = {0, 1, 5, 8, 9, 10, 17, 17, 20, 24, 30};
    int want[] = {0, 1, 5, 8, 10, 13, 17, 18, 22, 25, 30}; /* CLRS 14.1 列出的 r1..r10 */
    int first[11];
    for (int n = 1; n <= 10; n++) {
        assert(rod_cut(price, n, first) == want[n]);
        assert(rod_cut_memo(price, n) == want[n]);
    }
    /* 依 first_cut 還原切法，價錢加總要等於答案 */
    rod_cut(price, 7, first);
    int n = 7, sum = 0;
    while (n > 0) {
        sum += price[first[n]];
        n -= first[n];
    }
    assert(sum == 18);
}

static int rod_brute(const int *price, int n) /* 枚舉所有切法 */
{
    int best = 0;
    for (int i = 1; i <= n; i++) {
        int v = price[i] + rod_brute(price, n - i);
        if (v > best)
            best = v;
    }
    return best;
}

static void test_matrix_chain(void)
{
    /* CLRS Figure 14.5：答案 15125，括號 ((A1(A2A3))((A4A5)A6)) */
    int p[] = {30, 35, 15, 5, 10, 20, 25};
    int s[36];
    assert(matrix_chain(p, 6, s) == 15125);
    char out[64];
    matrix_chain_paren(s, 6, out);
    assert(strcmp(out, "((A1(A2A3))((A4A5)A6))") == 0);
    int q[] = {10, 100, 5, 50}; /* ((A1A2)A3) = 5000 + 2500 = 7500 */
    assert(matrix_chain(q, 3, s) == 7500);
}

static int lcs_brute(const char *x, const char *y)
{
    if (!*x || !*y)
        return 0;
    if (*x == *y)
        return 1 + lcs_brute(x + 1, y + 1);
    int a = lcs_brute(x + 1, y), b = lcs_brute(x, y + 1);
    return a > b ? a : b;
}

static int is_subseq(const char *s, const char *t) /* s 是不是 t 的子序列 */
{
    for (; *t && *s; t++)
        if (*s == *t)
            s++;
    return *s == '\0';
}

static int edit_brute(const char *a, const char *b)
{
    if (!*a)
        return (int)strlen(b);
    if (!*b)
        return (int)strlen(a);
    if (*a == *b)
        return edit_brute(a + 1, b + 1);
    int x = edit_brute(a + 1, b + 1), y = edit_brute(a + 1, b), z = edit_brute(a, b + 1);
    int m = x < y ? x : y;
    return 1 + (m < z ? m : z);
}

static void test_strings(void)
{
    char out[32];
    assert(lcs("ABCBDAB", "BDCABA", out) == 4 && is_subseq(out, "ABCBDAB") && is_subseq(out, "BDCABA"));
    assert(edit_distance("kitten", "sitting") == 3);
    assert(edit_distance("", "abc") == 3);
    srand(28);
    for (int t = 0; t < 500; t++) {
        char a[9], b[9];
        int n = rand() % 9, m = rand() % 9;
        for (int i = 0; i < n; i++)
            a[i] = (char)('a' + rand() % 3);
        for (int i = 0; i < m; i++)
            b[i] = (char)('a' + rand() % 3);
        a[n] = b[m] = '\0';
        int len = lcs(a, b, out);
        assert(len == lcs_brute(a, b) && (int)strlen(out) == len && is_subseq(out, a) && is_subseq(out, b));
        assert(edit_distance(a, b) == edit_brute(a, b));
    }
}

static void test_knapsack_lis_rod(void)
{
    int w[] = {1, 3, 4, 5}, v[] = {1, 4, 5, 7};
    assert(knapsack01(w, v, 4, 7) == 9); /* 拿 3 + 4 */
    srand(280);
    for (int t = 0; t < 500; t++) {
        int n = 1 + rand() % 10, ww[10], vv[10], cap = rand() % 30;
        for (int i = 0; i < n; i++) {
            ww[i] = 1 + rand() % 10;
            vv[i] = rand() % 20;
        }
        int best = 0; /* 枚舉 2^n 種拿法 */
        for (int mask = 0; mask < 1 << n; mask++) {
            int sw = 0, sv = 0;
            for (int i = 0; i < n; i++)
                if (mask >> i & 1) {
                    sw += ww[i];
                    sv += vv[i];
                }
            if (sw <= cap && sv > best)
                best = sv;
        }
        assert(knapsack01(ww, vv, n, cap) == best);

        int a[30], m = rand() % 30, dp[30], want = 0; /* LIS 對照 O(n²) DP */
        for (int i = 0; i < m; i++) {
            a[i] = rand() % 10;
            dp[i] = 1;
            for (int j = 0; j < i; j++)
                if (a[j] < a[i] && dp[j] + 1 > dp[i])
                    dp[i] = dp[j] + 1;
            if (dp[i] > want)
                want = dp[i];
        }
        assert(lis_length(a, m) == want);

        int price[13] = {0};
        for (int i = 1; i <= 12; i++)
            price[i] = rand() % 30;
        int L = 1 + rand() % 12;
        assert(rod_cut(price, L, NULL) == rod_brute(price, L));
    }
}

int main(void)
{
    test_rod();
    test_matrix_chain();
    test_strings();
    test_knapsack_lis_rod();
    puts("dp: all tests passed");
    return 0;
}
