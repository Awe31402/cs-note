#include "greedy.h"

#include <stdlib.h>
#include <string.h>

int activity_select(const int *s, const int *f, int n, int *chosen)
{
    int k = 0, last_end = -2147483647 - 1;
    for (int i = 0; i < n; i++)
        if (s[i] >= last_end) { /* 跟上一個選中的不衝突 */
            chosen[k++] = i;     /* 貪心：在相容的活動中，永遠選「最早結束」的 */
            last_end = f[i];
        }
    return k;
}

/* ---- Huffman：用 min-heap 反覆拿出最小的兩個合併 ---- */

typedef struct {
    long long w;
    int id; /* 節點編號：0..n-1 是葉子，n.. 是合併出來的內部節點 */
} HItem;

static void hpush(HItem *h, int *n, HItem x)
{
    int i = (*n)++;
    h[i] = x;
    while (i > 0 && h[(i - 1) / 2].w > h[i].w) {
        HItem t = h[i];
        h[i] = h[(i - 1) / 2];
        h[(i - 1) / 2] = t;
        i = (i - 1) / 2;
    }
}

static HItem hpop(HItem *h, int *n)
{
    HItem top = h[0];
    h[0] = h[--*n];
    for (int i = 0;;) {
        int l = 2 * i + 1, r = l + 1, s = i;
        if (l < *n && h[l].w < h[s].w)
            s = l;
        if (r < *n && h[r].w < h[s].w)
            s = r;
        if (s == i)
            break;
        HItem t = h[i];
        h[i] = h[s];
        h[s] = t;
        i = s;
    }
    return top;
}

long long huffman(const long long *freq, int n, char (*codes)[64])
{
    int total = 2 * n - 1, hs = 0;
    int *left = malloc((size_t)total * sizeof *left), *right = malloc((size_t)total * sizeof *right);
    HItem *h = malloc((size_t)n * sizeof *h);
    if (!left || !right || !h)
        abort();
    for (int i = 0; i < n; i++) {
        left[i] = right[i] = -1;
        hpush(h, &hs, (HItem){freq[i], i});
    }
    long long cost = 0;
    for (int id = n; id < total; id++) { /* n-1 次合併 */
        HItem a = hpop(h, &hs), b = hpop(h, &hs);
        left[id] = a.id;
        right[id] = b.id;
        cost += a.w + b.w; /* 每合併一次，底下所有字元的編碼都多 1 位 */
        hpush(h, &hs, (HItem){a.w + b.w, id});
    }
    /* 從根往下走：往左加 '0'、往右加 '1'（用 stack 模擬 DFS） */
    int *st = malloc((size_t)total * sizeof *st), top = 0;
    char (*path)[64] = malloc((size_t)total * sizeof *path);
    st[top++] = total - 1;
    path[total - 1][0] = '\0';
    while (top > 0) {
        int u = st[--top];
        if (u < n) {
            strcpy(codes[u], path[u]);
            continue;
        }
        size_t len = strlen(path[u]);
        memcpy(path[left[u]], path[u], len);
        path[left[u]][len] = '0';
        path[left[u]][len + 1] = '\0';
        memcpy(path[right[u]], path[u], len);
        path[right[u]][len] = '1';
        path[right[u]][len + 1] = '\0';
        st[top++] = left[u];
        st[top++] = right[u];
    }
    free(left);
    free(right);
    free(h);
    free(st);
    free(path);
    return cost;
}

/* ---- 分數背包 ---- */

static const int *g_w, *g_v;

static int by_ratio_desc(const void *a, const void *b)
{
    int i = *(const int *)a, j = *(const int *)b;
    /* v[i]/w[i] > v[j]/w[j] ⇔ v[i]·w[j] > v[j]·w[i]：交叉相乘，避開浮點數 */
    long long x = (long long)g_v[i] * g_w[j], y = (long long)g_v[j] * g_w[i];
    return (x < y) - (x > y);
}

double fractional_knapsack(const int *w, const int *v, int n, int cap)
{
    int *idx = malloc((size_t)(n ? n : 1) * sizeof *idx);
    if (!idx)
        abort();
    for (int i = 0; i < n; i++)
        idx[i] = i;
    g_w = w;
    g_v = v;
    qsort(idx, (size_t)n, sizeof *idx, by_ratio_desc);
    double total = 0;
    for (int k = 0; k < n && cap > 0; k++) {
        int i = idx[k];
        if (w[i] <= cap) { /* 整個拿 */
            total += v[i];
            cap -= w[i];
        } else { /* 只拿一部分，背包就滿了 */
            total += (double)v[i] * cap / w[i];
            cap = 0;
        }
    }
    free(idx);
    return total;
}

/* ---- 離線快取 ---- */

int offline_cache_misses(const int *req, int m, int k)
{
    int *cache = malloc((size_t)k * sizeof *cache), used = 0, misses = 0;
    if (!cache)
        abort();
    for (int t = 0; t < m; t++) {
        int hit = 0;
        for (int i = 0; i < used; i++)
            if (cache[i] == req[t])
                hit = 1;
        if (hit)
            continue;
        misses++;
        if (used < k) {
            cache[used++] = req[t];
            continue;
        }
        int victim = 0, far = -1;
        for (int i = 0; i < used; i++) { /* 找「下次出現最晚」（或再也不出現）的 */
            int next = m;
            for (int u = t + 1; u < m; u++)
                if (req[u] == cache[i]) {
                    next = u;
                    break;
                }
            if (next > far) {
                far = next;
                victim = i;
            }
        }
        cache[victim] = req[t];
    }
    free(cache);
    return misses;
}
