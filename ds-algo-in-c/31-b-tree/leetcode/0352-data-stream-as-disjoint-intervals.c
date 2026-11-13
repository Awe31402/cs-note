/* LeetCode 352 · Data Stream as Disjoint Intervals
 * 數字一個一個加進來（0..10^4，可能重複），隨時要回傳「目前所有數字合併成的不相交區間」。
 * 思路：用 B-Tree 當有序集合存「出現過的數字」。getIntervals 時中序走訪（由小到大），
 *       把連續的數字併成一段。addNum O(log n)，getIntervals O(n)。
 *       題目：addNum 最多 3×10^4 次、getIntervals 最多 10^2 次，所以 get 慢一點沒關係。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
/* ---- 精簡版 B-Tree（CLRS 第 18 章，只有插入）：t = 4，每個節點最多 7 個 key ---- */
#define T 4

typedef struct BN {
    int n, leaf;
    int key[2 * T - 1], val[2 * T - 1];
    struct BN *c[2 * T];
} BN;

static BN *bn_new(int leaf)
{
    BN *x = calloc(1, sizeof *x);
    x->leaf = leaf;
    return x;
}

static void bn_free(BN *x)
{
    if (!x->leaf)
        for (int i = 0; i <= x->n; i++)
            bn_free(x->c[i]);
    free(x);
}

static void split(BN *x, int i) /* x->c[i] 是滿的：中間的 key 升到 x */
{
    BN *y = x->c[i], *z = bn_new(y->leaf);
    z->n = T - 1;
    for (int j = 0; j < T - 1; j++) {
        z->key[j] = y->key[j + T];
        z->val[j] = y->val[j + T];
    }
    if (!y->leaf)
        for (int j = 0; j < T; j++)
            z->c[j] = y->c[j + T];
    y->n = T - 1;
    for (int j = x->n; j > i; j--)
        x->c[j + 1] = x->c[j];
    x->c[i + 1] = z;
    for (int j = x->n - 1; j >= i; j--) {
        x->key[j + 1] = x->key[j];
        x->val[j + 1] = x->val[j];
    }
    x->key[i] = y->key[T - 1];
    x->val[i] = y->val[T - 1];
    x->n++;
}

static void ins_nonfull(BN *x, int k, int v)
{
    int i = x->n - 1;
    if (x->leaf) {
        while (i >= 0 && k < x->key[i]) {
            x->key[i + 1] = x->key[i];
            x->val[i + 1] = x->val[i];
            i--;
        }
        x->key[i + 1] = k;
        x->val[i + 1] = v;
        x->n++;
        return;
    }
    while (i >= 0 && k < x->key[i])
        i--;
    i++;
    if (x->c[i]->n == 2 * T - 1) {
        split(x, i);
        if (k > x->key[i])
            i++;
    }
    ins_nonfull(x->c[i], k, v);
}

static BN *bt_insert(BN *root, int k, int v) /* 回傳新的根；呼叫前要確定 k 不存在 */
{
    if (root->n == 2 * T - 1) {
        BN *s = bn_new(0);
        s->c[0] = root;
        split(s, 0);
        root = s;
    }
    ins_nonfull(root, k, v);
    return root;
}

static int bt_contains(const BN *x, int k)
{
    for (;;) {
        int i = 0;
        while (i < x->n && k > x->key[i])
            i++;
        if (i < x->n && x->key[i] == k)
            return 1;
        if (x->leaf)
            return 0;
        x = x->c[i];
    }
}

typedef struct {
    BN *root;
    int size;
} SummaryRanges;

SummaryRanges *summaryRangesCreate(void)
{
    SummaryRanges *s = malloc(sizeof *s);
    s->root = bn_new(1);
    s->size = 0;
    return s;
}

void summaryRangesAddNum(SummaryRanges *s, int value)
{
    if (bt_contains(s->root, value))
        return;
    s->root = bt_insert(s->root, value, 0);
    s->size++;
}

/* 中序走訪，邊走邊合併：跟上一段的尾巴相連（+1）就延長，否則開新的一段 */
static void walk(const BN *x, int **out, int *cnt)
{
    for (int i = 0; i <= x->n; i++) {
        if (!x->leaf)
            walk(x->c[i], out, cnt);
        if (i == x->n)
            break;
        int v = x->key[i];
        if (*cnt > 0 && out[*cnt - 1][1] + 1 == v) {
            out[*cnt - 1][1] = v;
        } else {
            out[*cnt] = malloc(2 * sizeof **out);
            out[*cnt][0] = out[*cnt][1] = v;
            (*cnt)++;
        }
    }
}

int **summaryRangesGetIntervals(SummaryRanges *s, int *retSize, int **retColSize)
{
    int **out = malloc((size_t)(s->size ? s->size : 1) * sizeof *out), cnt = 0;
    walk(s->root, out, &cnt);
    *retSize = cnt;
    *retColSize = malloc((size_t)(cnt ? cnt : 1) * sizeof **retColSize);
    for (int i = 0; i < cnt; i++)
        (*retColSize)[i] = 2;
    return out;
}

void summaryRangesFree(SummaryRanges *s)
{
    bn_free(s->root);
    free(s);
}
/* ===== 提交範圍 結束 ===== */

static void check(SummaryRanges *s, const int (*want)[2], int k)
{
    int n, *cols;
    int **r = summaryRangesGetIntervals(s, &n, &cols);
    assert(n == k);
    for (int i = 0; i < n; i++) {
        assert(cols[i] == 2 && r[i][0] == want[i][0] && r[i][1] == want[i][1]);
        free(r[i]);
    }
    free(r);
    free(cols);
}

int main(void)
{
    SummaryRanges *s = summaryRangesCreate();
    summaryRangesAddNum(s, 1);
    check(s, (const int[][2]){{1, 1}}, 1);
    summaryRangesAddNum(s, 3);
    check(s, (const int[][2]){{1, 1}, {3, 3}}, 2);
    summaryRangesAddNum(s, 7);
    summaryRangesAddNum(s, 2);
    check(s, (const int[][2]){{1, 3}, {7, 7}}, 2);
    summaryRangesAddNum(s, 6);
    summaryRangesAddNum(s, 6); /* 重複 */
    check(s, (const int[][2]){{1, 3}, {6, 7}}, 2);
    summaryRangesFree(s);

    /* 隨機：大量加入（讓 B-Tree 長好幾層），對照布林陣列 */
    static char seen[10001];
    srand(352);
    s = summaryRangesCreate();
    for (int t = 0; t < 20000; t++) {
        int v = rand() % 10001;
        seen[v] = 1;
        summaryRangesAddNum(s, v);
    }
    int want[10001][2], k = 0;
    for (int v = 0; v <= 10000; v++) {
        if (!seen[v])
            continue;
        if (k > 0 && want[k - 1][1] + 1 == v)
            want[k - 1][1] = v;
        else {
            want[k][0] = want[k][1] = v;
            k++;
        }
    }
    check(s, (const int(*)[2])want, k);
    summaryRangesFree(s);
    puts("0352: passed");
    return 0;
}
