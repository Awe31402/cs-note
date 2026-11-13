/* LeetCode 729 · My Calendar I（B-Tree 版）
 * 18 主題用紅黑樹解過。這裡換成 B-Tree 當有序集合：key = start、val = end。
 * 新的 [s, e) 只要檢查兩個鄰居：第一個 start >= s 的（它的 start 要 >= e）、
 * 最後一個 start < s 的（它的 end 要 <= s）。每次 O(log n)。
 */
#include <assert.h>
#include <stdbool.h>
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

/* 第一個 key >= k：回傳 1 並寫入 *key、*val；沒有回傳 0 */
static int bt_lower(const BN *x, int k, int *key, int *val)
{
    int found = 0;
    for (;;) {
        int i = 0;
        while (i < x->n && k > x->key[i])
            i++;
        if (i < x->n) { /* 候選；更小的候選只會在 c[i] 裡 */
            *key = x->key[i];
            *val = x->val[i];
            found = 1;
        }
        if (x->leaf)
            return found;
        x = x->c[i];
    }
}

/* 最後一個 key < k */
static int bt_before(const BN *x, int k, int *key, int *val)
{
    int found = 0;
    for (;;) {
        int i = 0;
        while (i < x->n && x->key[i] < k)
            i++;
        if (i > 0) { /* key[i-1] < k 是候選；更大的候選只會在 c[i] 裡 */
            *key = x->key[i - 1];
            *val = x->val[i - 1];
            found = 1;
        }
        if (x->leaf)
            return found;
        x = x->c[i];
    }
}

typedef struct {
    BN *root;
} MyCalendar;

MyCalendar *myCalendarCreate(void)
{
    MyCalendar *c = malloc(sizeof *c);
    c->root = bn_new(1);
    return c;
}

bool myCalendarBook(MyCalendar *c, int startTime, int endTime)
{
    int k, v;
    if (bt_lower(c->root, startTime, &k, &v) && k < endTime)
        return false;
    if (bt_before(c->root, startTime, &k, &v) && v > startTime)
        return false;
    c->root = bt_insert(c->root, startTime, endTime);
    return true;
}

void myCalendarFree(MyCalendar *c)
{
    bn_free(c->root);
    free(c);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    MyCalendar *c = myCalendarCreate();
    assert(myCalendarBook(c, 10, 20));
    assert(!myCalendarBook(c, 15, 25));
    assert(myCalendarBook(c, 20, 30));
    assert(!myCalendarBook(c, 5, 11));
    assert(myCalendarBook(c, 5, 10));
    myCalendarFree(c);

    srand(7290);
    c = myCalendarCreate();
    static int s[3000], e[3000];
    int n = 0;
    for (int k = 0; k < 3000; k++) {
        int a = rand() % 20000, b = a + 1 + rand() % 10;
        int ok = 1;
        for (int i = 0; i < n; i++)
            if (a < e[i] && s[i] < b)
                ok = 0;
        assert(myCalendarBook(c, a, b) == ok);
        if (ok) {
            s[n] = a;
            e[n++] = b;
        }
    }
    assert(n > 500); /* 確認 B-Tree 真的長了好幾層 */
    myCalendarFree(c);
    puts("0729: passed");
    return 0;
}
