#include "dp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *xcalloc(size_t n, size_t sz)
{
    void *p = calloc(n ? n : 1, sz);
    if (!p)
        abort();
    return p;
}

/* r[j] = 長度 j 能賣的最多錢 = max over i (price[i] + r[j - i])：第一刀切下長度 i，剩下的最佳切法已經算好 */
int rod_cut(const int *price, int n, int *first_cut)
{
    int *r = xcalloc((size_t)n + 1, sizeof *r);
    for (int j = 1; j <= n; j++) {
        int best = -1;
        for (int i = 1; i <= j; i++)
            if (price[i] + r[j - i] > best) {
                best = price[i] + r[j - i];
                if (first_cut)
                    first_cut[j] = i;
            }
        r[j] = best;
    }
    int ans = r[n];
    free(r);
    return ans;
}

static int rod_rec(const int *price, int n, int *memo)
{
    if (n == 0)
        return 0;
    if (memo[n] >= 0)
        return memo[n]; /* 算過了：直接拿 */
    int best = -1;
    for (int i = 1; i <= n; i++) {
        int v = price[i] + rod_rec(price, n - i, memo);
        if (v > best)
            best = v;
    }
    return memo[n] = best;
}

int rod_cut_memo(const int *price, int n)
{
    int *memo = malloc(((size_t)n + 1) * sizeof *memo);
    if (!memo)
        abort();
    memset(memo, -1, ((size_t)n + 1) * sizeof *memo);
    int ans = rod_rec(price, n, memo);
    free(memo);
    return ans;
}

/* m[i][j] = A_i..A_j 的最少乘法次數
 *        = min over i <= k < j ( m[i][k] + m[k+1][j] + p[i-1]·p[k]·p[j] )
 * 依「鏈的長度」由短到長填表：算長鏈時，短鏈都算好了。 */
long long matrix_chain(const int *p, int n, int *s)
{
    long long *m = xcalloc((size_t)n * n, sizeof *m);
#define M(i, j) m[((i) - 1) * n + ((j) - 1)]
    for (int len = 2; len <= n; len++)
        for (int i = 1; i + len - 1 <= n; i++) {
            int j = i + len - 1;
            M(i, j) = -1;
            for (int k = i; k < j; k++) {
                long long q = M(i, k) + M(k + 1, j) + (long long)p[i - 1] * p[k] * p[j];
                if (M(i, j) < 0 || q < M(i, j)) {
                    M(i, j) = q;
                    if (s)
                        s[(i - 1) * n + (j - 1)] = k;
                }
            }
        }
    long long ans = n > 0 ? M(1, n) : 0;
#undef M
    free(m);
    return ans;
}

static int paren_rec(const int *s, int n, int i, int j, char *out, int k)
{
    if (i == j)
        return k + sprintf(out + k, "A%d", i);
    int split = s[(i - 1) * n + (j - 1)];
    out[k++] = '(';
    k = paren_rec(s, n, i, split, out, k);
    k = paren_rec(s, n, split + 1, j, out, k);
    out[k++] = ')';
    out[k] = '\0';
    return k;
}

int matrix_chain_paren(const int *s, int n, char *out)
{
    return paren_rec(s, n, 1, n, out, 0);
}

/* c[i][j] = x[0..i) 和 y[0..j) 的 LCS 長度
 *   x[i-1] == y[j-1] → c[i-1][j-1] + 1（這個字元一定可以放進去）
 *   否則            → max(c[i-1][j], c[i][j-1])（丟掉其中一邊的最後一個字元） */
int lcs(const char *x, const char *y, char *out)
{
    int n = (int)strlen(x), m = (int)strlen(y);
    int *c = xcalloc((size_t)(n + 1) * (m + 1), sizeof *c);
#define C(i, j) c[(i) * (m + 1) + (j)]
    for (int i = 1; i <= n; i++)
        for (int j = 1; j <= m; j++)
            C(i, j) = x[i - 1] == y[j - 1] ? C(i - 1, j - 1) + 1 : (C(i - 1, j) > C(i, j - 1) ? C(i - 1, j) : C(i, j - 1));
    int len = C(n, m);
    if (out) { /* 從右下角沿著「是從哪裡來的」往回走，還原出一個 LCS */
        int i = n, j = m, k = len;
        out[k] = '\0';
        while (i > 0 && j > 0) {
            if (x[i - 1] == y[j - 1]) {
                out[--k] = x[i - 1];
                i--;
                j--;
            } else if (C(i - 1, j) >= C(i, j - 1)) {
                i--;
            } else {
                j--;
            }
        }
    }
#undef C
    free(c);
    return len;
}

int edit_distance(const char *a, const char *b)
{
    int n = (int)strlen(a), m = (int)strlen(b);
    int *prev = xcalloc((size_t)m + 1, sizeof *prev), *cur = xcalloc((size_t)m + 1, sizeof *cur);
    for (int j = 0; j <= m; j++)
        prev[j] = j; /* 空字串變成 b[0..j)：插入 j 次 */
    for (int i = 1; i <= n; i++) {
        cur[0] = i; /* a[0..i) 變成空字串：刪除 i 次 */
        for (int j = 1; j <= m; j++) {
            if (a[i - 1] == b[j - 1]) {
                cur[j] = prev[j - 1]; /* 最後一個字元一樣：不用動 */
            } else {
                int best = prev[j - 1]; /* 替換 */
                if (prev[j] < best)
                    best = prev[j]; /* 刪除 a[i-1] */
                if (cur[j - 1] < best)
                    best = cur[j - 1]; /* 插入 b[j-1] */
                cur[j] = best + 1;
            }
        }
        int *t = prev; /* 只需要上一列：空間 O(m) */
        prev = cur;
        cur = t;
    }
    int ans = prev[m];
    free(prev);
    free(cur);
    return ans;
}

int knapsack01(const int *w, const int *v, int n, int cap)
{
    int *best = xcalloc((size_t)cap + 1, sizeof *best); /* best[c] = 容量 c 能拿的最大價值 */
    for (int i = 0; i < n; i++)
        for (int c = cap; c >= w[i]; c--) /* 由大到小：每樣東西只會被用一次 */
            if (best[c - w[i]] + v[i] > best[c])
                best[c] = best[c - w[i]] + v[i];
    int ans = best[cap];
    free(best);
    return ans;
}

/* tails[k] = 長度 k+1 的遞增子序列中，最小可能的結尾。tails 是遞增的，所以能二分搜尋。 */
int lis_length(const int *a, int n)
{
    int *tails = xcalloc((size_t)n, sizeof *tails), len = 0;
    for (int i = 0; i < n; i++) {
        int lo = 0, hi = len;
        while (lo < hi) { /* 第一個 >= a[i] 的位置 */
            int mid = lo + (hi - lo) / 2;
            if (tails[mid] >= a[i])
                hi = mid;
            else
                lo = mid + 1;
        }
        tails[lo] = a[i]; /* 接在後面（lo == len）或換成更小的結尾 */
        if (lo == len)
            len++;
    }
    free(tails);
    return len;
}
