#include "linear_sort.h"

#include <stdlib.h>
#include <string.h>

static void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p)
        abort();
    return p;
}

void counting_sort(const int *in, int n, int *out)
{
    if (n == 0)
        return;
    int lo = in[0], hi = in[0];
    for (int i = 1; i < n; i++) {
        if (in[i] < lo)
            lo = in[i];
        if (in[i] > hi)
            hi = in[i];
    }
    size_t k = (size_t)((long long)hi - lo + 1);
    int *cnt = calloc(k, sizeof *cnt);
    if (!cnt)
        abort();
    for (int i = 0; i < n; i++)
        cnt[in[i] - lo]++; /* 1. 數每個值出現幾次 */
    for (size_t v = 1; v < k; v++)
        cnt[v] += cnt[v - 1]; /* 2. 前綴和：cnt[v] = 「<= v」的個數 = v 最後一個的位置 + 1 */
    for (int i = n - 1; i >= 0; i--)
        out[--cnt[in[i] - lo]] = in[i]; /* 3. 從後往前放：穩定 */
    free(cnt);
}

void counting_sort_rec(const Rec *in, int n, int max_key, Rec *out)
{
    int *cnt = calloc((size_t)max_key + 1, sizeof *cnt);
    if (!cnt)
        abort();
    for (int i = 0; i < n; i++)
        cnt[in[i].key]++;
    for (int v = 1; v <= max_key; v++)
        cnt[v] += cnt[v - 1];
    for (int i = n - 1; i >= 0; i--) /* 從後往前：同 key 裡後面的放到後面 */
        out[--cnt[in[i].key]] = in[i];
    free(cnt);
}

void radix_sort(int *a, int n)
{
    /* 把最高位元（正負號）反轉：負數變小的無號數、正數變大的，無號大小順序 = 原本有號的順序 */
    unsigned *u = xmalloc((size_t)n * sizeof *u), *tmp = xmalloc((size_t)n * sizeof *tmp);
    for (int i = 0; i < n; i++)
        u[i] = (unsigned)a[i] ^ 0x80000000u;
    for (int shift = 0; shift < 32; shift += 8) { /* 從最低的 byte 開始 (LSD) */
        int cnt[256] = {0};
        for (int i = 0; i < n; i++)
            cnt[(u[i] >> shift) & 0xFF]++;
        for (int d = 1; d < 256; d++)
            cnt[d] += cnt[d - 1];
        for (int i = n - 1; i >= 0; i--) /* 每一趟都必須穩定，前面幾趟排好的順序才不會被打亂 */
            tmp[--cnt[(u[i] >> shift) & 0xFF]] = u[i];
        unsigned *t = u;
        u = tmp;
        tmp = t;
    }
    for (int i = 0; i < n; i++)
        a[i] = (int)(u[i] ^ 0x80000000u);
    free(u);
    free(tmp);
}

void bucket_sort(double *a, int n)
{
    if (n == 0)
        return;
    /* 先數每桶幾個，再放進一個大陣列裡的對應區段（避免每桶一條串列） */
    int *start = calloc((size_t)n + 1, sizeof *start);
    double *b = xmalloc((size_t)n * sizeof *b);
    if (!start)
        abort();
    for (int i = 0; i < n; i++)
        start[(int)(a[i] * n) + 1]++;
    for (int k = 1; k <= n; k++)
        start[k] += start[k - 1];
    int *fill = xmalloc((size_t)n * sizeof *fill);
    memcpy(fill, start, (size_t)n * sizeof *fill);
    for (int i = 0; i < n; i++)
        b[fill[(int)(a[i] * n)]++] = a[i];
    for (int k = 0; k < n; k++) /* 每一桶各自插入排序；平均每桶 O(1) 個 */
        for (int i = start[k] + 1; i < start[k + 1]; i++) {
            double key = b[i];
            int j = i - 1;
            while (j >= start[k] && b[j] > key) {
                b[j + 1] = b[j];
                j--;
            }
            b[j + 1] = key;
        }
    memcpy(a, b, (size_t)n * sizeof *a);
    free(start);
    free(fill);
    free(b);
}
