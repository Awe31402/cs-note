#ifndef SEGTREE_H
#define SEGTREE_H

/* 線段樹 (segment tree)：區間加值 + 區間求和，用懶標記 (lazy propagation)。
 * 節點 1 是根，管 [0, n-1]；節點 k 的左右小孩是 2k、2k+1（跟 heap 一樣的編號法）。 */
typedef struct {
    int n;
    long long *sum;  /* 這個節點管的區間總和 */
    long long *lazy; /* 「還沒往下傳」的加值：整個區間每個元素都要再加這麼多 */
} SegTree;

void      st_init(SegTree *t, const int *a, int n); /* O(n) 建樹 */
void      st_free(SegTree *t);
void      st_add(SegTree *t, int l, int r, long long v); /* a[l..r] 每個都 += v，O(log n) */
long long st_sum(SegTree *t, int l, int r);              /* a[l..r] 的和，O(log n) */
void      st_set(SegTree *t, int i, long long v);        /* a[i] = v，O(log n) */

/* 樹狀陣列 (Fenwick tree / Binary Indexed Tree)：只支援「單點加值 + 前綴和」，但程式超短、常數很小。
 * 內部用 1-based。 */
typedef struct {
    int n;
    long long *bit;
} Fenwick;

void      fw_init(Fenwick *f, int n);
void      fw_free(Fenwick *f);
void      fw_add(Fenwick *f, int i, long long v); /* a[i] += v（i 是 0-based），O(log n) */
long long fw_prefix(const Fenwick *f, int i);     /* a[0..i] 的和，i = -1 回傳 0，O(log n) */

#endif
