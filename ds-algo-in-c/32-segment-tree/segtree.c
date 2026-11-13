#include "segtree.h"

#include <stdlib.h>

static void build(SegTree *t, const int *a, int k, int lo, int hi)
{
    if (lo == hi) {
        t->sum[k] = a[lo];
        return;
    }
    int mid = lo + (hi - lo) / 2;
    build(t, a, 2 * k, lo, mid);
    build(t, a, 2 * k + 1, mid + 1, hi);
    t->sum[k] = t->sum[2 * k] + t->sum[2 * k + 1];
}

void st_init(SegTree *t, const int *a, int n)
{
    t->n = n;
    /* 4n 格一定夠：樹高 ceil(log2 n) + 1，節點編號最大不超過 4n */
    t->sum = calloc((size_t)(4 * (n > 0 ? n : 1)), sizeof *t->sum);
    t->lazy = calloc((size_t)(4 * (n > 0 ? n : 1)), sizeof *t->lazy);
    if (!t->sum || !t->lazy)
        abort();
    if (n > 0)
        build(t, a, 1, 0, n - 1);
}

void st_free(SegTree *t)
{
    free(t->sum);
    free(t->lazy);
    t->n = 0;
}

/* 節點 k 管 [lo, hi]，整段都加 v：只更新自己的 sum，並把「欠小孩的」記在 lazy */
static void apply(SegTree *t, int k, int lo, int hi, long long v)
{
    t->sum[k] += v * (hi - lo + 1);
    t->lazy[k] += v;
}

/* 要往下走之前，把欠的加值傳給兩個小孩 */
static void push_down(SegTree *t, int k, int lo, int hi)
{
    if (t->lazy[k] == 0)
        return;
    int mid = lo + (hi - lo) / 2;
    apply(t, 2 * k, lo, mid, t->lazy[k]);
    apply(t, 2 * k + 1, mid + 1, hi, t->lazy[k]);
    t->lazy[k] = 0;
}

static void add_rec(SegTree *t, int k, int lo, int hi, int l, int r, long long v)
{
    if (r < lo || hi < l)
        return; /* 完全不相交 */
    if (l <= lo && hi <= r) {
        apply(t, k, lo, hi, v); /* 完全包含：在這裡停，不用往下走 → 這就是 O(log n) 的關鍵 */
        return;
    }
    push_down(t, k, lo, hi);
    int mid = lo + (hi - lo) / 2;
    add_rec(t, 2 * k, lo, mid, l, r, v);
    add_rec(t, 2 * k + 1, mid + 1, hi, l, r, v);
    t->sum[k] = t->sum[2 * k] + t->sum[2 * k + 1];
}

static long long sum_rec(SegTree *t, int k, int lo, int hi, int l, int r)
{
    if (r < lo || hi < l)
        return 0;
    if (l <= lo && hi <= r)
        return t->sum[k];
    push_down(t, k, lo, hi);
    int mid = lo + (hi - lo) / 2;
    return sum_rec(t, 2 * k, lo, mid, l, r) + sum_rec(t, 2 * k + 1, mid + 1, hi, l, r);
}

void st_add(SegTree *t, int l, int r, long long v)
{
    add_rec(t, 1, 0, t->n - 1, l, r, v);
}

long long st_sum(SegTree *t, int l, int r)
{
    return sum_rec(t, 1, 0, t->n - 1, l, r);
}

void st_set(SegTree *t, int i, long long v)
{
    st_add(t, i, i, v - st_sum(t, i, i)); /* 單點設值 = 加上差值 */
}

/* ---- Fenwick ---- */

void fw_init(Fenwick *f, int n)
{
    f->n = n;
    f->bit = calloc((size_t)n + 1, sizeof *f->bit);
    if (!f->bit)
        abort();
}

void fw_free(Fenwick *f)
{
    free(f->bit);
    f->n = 0;
}

/* i & -i = i 最低的那個 1 位元。bit[i] 管的是 (i - lowbit(i), i] 這一段 */
void fw_add(Fenwick *f, int i, long long v)
{
    for (i++; i <= f->n; i += i & -i)
        f->bit[i] += v;
}

long long fw_prefix(const Fenwick *f, int i)
{
    long long s = 0;
    for (i++; i > 0; i -= i & -i)
        s += f->bit[i];
    return s;
}
