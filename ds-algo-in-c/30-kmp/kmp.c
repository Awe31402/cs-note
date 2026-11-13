#include "kmp.h"

#include <stdlib.h>
#include <string.h>

int naive_match(const char *t, const char *p, int *pos)
{
    int n = (int)strlen(t), m = (int)strlen(p), k = 0;
    for (int s = 0; s + m <= n; s++) {
        int j = 0;
        while (j < m && t[s + j] == p[j])
            j++;
        if (j == m)
            pos[k++] = s;
    }
    return k;
}

int rabin_karp(const char *t, const char *p, int *pos)
{
    int n = (int)strlen(t), m = (int)strlen(p), k = 0;
    if (m > n)
        return 0;
    const unsigned long long B = 256, Q = 1000000007ULL; /* 基底、質數模數 */
    unsigned long long hp = 0, ht = 0, h = 1;            /* h = B^(m-1) mod Q：滑動時要扣掉最左邊的字元 */
    for (int i = 0; i < m - 1; i++)
        h = h * B % Q;
    for (int i = 0; i < m; i++) {
        hp = (hp * B + (unsigned char)p[i]) % Q;
        ht = (ht * B + (unsigned char)t[i]) % Q;
    }
    for (int s = 0;; s++) {
        /* 雜湊相同還要逐字比對：不同字串可能碰巧雜湊值一樣（spurious hit） */
        if (hp == ht && memcmp(t + s, p, (size_t)m) == 0)
            pos[k++] = s;
        if (s + m >= n)
            break;
        /* 滾動：去掉 t[s]，加上 t[s+m]。先加 Q 避免無號數相減變成很大的數 */
        ht = (ht + Q - (unsigned char)t[s] * h % Q) % Q;
        ht = (ht * B + (unsigned char)t[s + m]) % Q;
    }
    return k;
}

void kmp_prefix(const char *p, int m, int *pi)
{
    if (m == 0)
        return;
    pi[0] = 0;
    int k = 0; /* 目前「前綴 = 後綴」的長度 */
    for (int q = 1; q < m; q++) {
        while (k > 0 && p[k] != p[q])
            k = pi[k - 1]; /* 接不下去：退到「次長」的前綴 = 後綴 */
        if (p[k] == p[q])
            k++;
        pi[q] = k;
    }
}

int kmp_match(const char *t, const char *p, int *pos)
{
    int n = (int)strlen(t), m = (int)strlen(p), cnt = 0;
    if (m == 0) { /* 空樣式：每個位置都算出現（跟 naive 一致） */
        for (int s = 0; s <= n; s++)
            pos[cnt++] = s;
        return cnt;
    }
    int *pi = malloc((size_t)m * sizeof *pi);
    if (!pi)
        abort();
    kmp_prefix(p, m, pi);
    int q = 0; /* 目前已經對上幾個字元 */
    for (int i = 0; i < n; i++) {
        while (q > 0 && p[q] != t[i])
            q = pi[q - 1]; /* 不用回頭看 T：直接用 pi 決定樣式要滑到哪 */
        if (p[q] == t[i])
            q++;
        if (q == m) {
            pos[cnt++] = i - m + 1;
            q = pi[q - 1]; /* 繼續找下一個（允許重疊） */
        }
    }
    free(pi);
    return cnt;
}

void z_function(const char *s, int n, int *z)
{
    if (n == 0)
        return;
    z[0] = n;
    int l = 0, r = 0; /* [l, r) = 目前已知、跟 s 開頭相同、右端最遠的區段 */
    for (int i = 1; i < n; i++) {
        z[i] = 0;
        if (i < r)
            z[i] = z[i - l] < r - i ? z[i - l] : r - i; /* 在區段內：直接借用之前算過的值 */
        while (i + z[i] < n && s[z[i]] == s[i + z[i]])
            z[i]++;
        if (i + z[i] > r) {
            l = i;
            r = i + z[i];
        }
    }
}
