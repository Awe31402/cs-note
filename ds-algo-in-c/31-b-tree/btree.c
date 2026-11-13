#include "btree.h"

#include <limits.h>
#include <stdlib.h>

static BTNode *node_new(int t, int leaf)
{
    BTNode *x = malloc(sizeof *x);
    if (!x)
        abort();
    x->n = 0;
    x->leaf = leaf;
    x->keys = malloc((size_t)(2 * t - 1) * sizeof *x->keys);
    x->child = calloc((size_t)(2 * t), sizeof *x->child);
    if (!x->keys || !x->child)
        abort();
    return x;
}

static void node_free(BTNode *x)
{
    free(x->keys);
    free(x->child);
    free(x);
}

void bt_init(BTree *b, int t)
{
    b->t = t;
    b->root = node_new(t, 1);
    b->size = 0;
}

static void free_rec(BTNode *x)
{
    if (!x->leaf)
        for (int i = 0; i <= x->n; i++)
            free_rec(x->child[i]);
    node_free(x);
}

void bt_free(BTree *b)
{
    free_rec(b->root);
    b->root = NULL;
    b->size = 0;
}

int bt_search(const BTree *b, int key)
{
    const BTNode *x = b->root;
    for (;;) {
        int i = 0;
        while (i < x->n && key > x->keys[i]) /* 節點裡的 key 不多時線性找就好；t 很大可改二分 */
            i++;
        if (i < x->n && key == x->keys[i])
            return 1;
        if (x->leaf)
            return 0;
        x = x->child[i]; /* keys[i-1] < key < keys[i] 的範圍在 child[i] */
    }
}

int bt_lower_bound(const BTree *b, int key, int *out)
{
    const BTNode *x = b->root;
    int found = 0;
    for (;;) {
        int i = 0;
        while (i < x->n && key > x->keys[i])
            i++;
        if (i < x->n) { /* keys[i] >= key 是候選；更小的候選只可能在 child[i] 裡 */
            *out = x->keys[i];
            found = 1;
            if (x->keys[i] == key)
                return 1;
        }
        if (x->leaf)
            return found;
        x = x->child[i];
    }
}

/* CLRS B-TREE-SPLIT-CHILD：x->child[i] 是滿的 (2t-1 個 key)，把中間的 key 提到 x，
 * 左右各留 t-1 個。
 *     x: [ ... P ... ]              x: [ ... M P ... ]
 *              |           →                /   \
 *     y: [a b c M d e f]           y: [a b c]   z: [d e f]      (t = 4) */
static void split_child(BTNode *x, int i, int t)
{
    BTNode *y = x->child[i], *z = node_new(t, y->leaf);
    z->n = t - 1;
    for (int j = 0; j < t - 1; j++)
        z->keys[j] = y->keys[j + t];
    if (!y->leaf)
        for (int j = 0; j < t; j++)
            z->child[j] = y->child[j + t];
    y->n = t - 1;
    for (int j = x->n; j > i; j--)
        x->child[j + 1] = x->child[j];
    x->child[i + 1] = z;
    for (int j = x->n - 1; j >= i; j--)
        x->keys[j + 1] = x->keys[j];
    x->keys[i] = y->keys[t - 1]; /* 中間那個升上來 */
    x->n++;
}

/* x 保證不是滿的。往下走之前，如果下一層的小孩是滿的就先分裂：一趟就能插完，不用回頭 */
static void insert_nonfull(BTNode *x, int key, int t)
{
    int i = x->n - 1;
    if (x->leaf) {
        while (i >= 0 && key < x->keys[i]) {
            x->keys[i + 1] = x->keys[i];
            i--;
        }
        x->keys[i + 1] = key;
        x->n++;
        return;
    }
    while (i >= 0 && key < x->keys[i])
        i--;
    i++;
    if (x->child[i]->n == 2 * t - 1) {
        split_child(x, i, t);
        if (key > x->keys[i]) /* 分裂後升上來的 key 決定要走左邊還右邊 */
            i++;
    }
    insert_nonfull(x->child[i], key, t);
}

int bt_insert(BTree *b, int key)
{
    if (bt_search(b, key))
        return 0;
    int t = b->t;
    BTNode *r = b->root;
    if (r->n == 2 * t - 1) { /* 根滿了：長出新的根。B-Tree 只會從「根」長高 */
        BTNode *s = node_new(t, 0);
        s->child[0] = r;
        b->root = s;
        split_child(s, 0, t);
        insert_nonfull(s, key, t);
    } else {
        insert_nonfull(r, key, t);
    }
    b->size++;
    return 1;
}

/* 把 x->keys[i] 和 child[i+1] 併進 child[i]，x 少一個 key 和一個小孩 */
static void merge(BTNode *x, int i)
{
    BTNode *y = x->child[i], *z = x->child[i + 1];
    int yn = y->n;
    y->keys[yn] = x->keys[i];
    for (int j = 0; j < z->n; j++)
        y->keys[yn + 1 + j] = z->keys[j];
    if (!y->leaf)
        for (int j = 0; j <= z->n; j++)
            y->child[yn + 1 + j] = z->child[j];
    y->n = yn + 1 + z->n;
    for (int j = i; j < x->n - 1; j++)
        x->keys[j] = x->keys[j + 1];
    for (int j = i + 1; j < x->n; j++)
        x->child[j] = x->child[j + 1];
    x->n--;
    node_free(z);
}

/* CLRS 18.3：往下走的每個節點都至少有 t 個 key（根除外），所以刪一個 key 不會讓它違規 */
static void delete_rec(BTNode *x, int key, int t)
{
    int i = 0;
    while (i < x->n && key > x->keys[i])
        i++;
    if (i < x->n && x->keys[i] == key) {
        if (x->leaf) { /* 情況 1：在葉子裡，直接刪 */
            for (int j = i; j < x->n - 1; j++)
                x->keys[j] = x->keys[j + 1];
            x->n--;
            return;
        }
        BTNode *y = x->child[i], *z = x->child[i + 1];
        if (y->n >= t) { /* 情況 2a：左小孩夠多 → 用前驅取代，再到左邊刪前驅 */
            BTNode *p = y;
            while (!p->leaf)
                p = p->child[p->n];
            int pred = p->keys[p->n - 1];
            x->keys[i] = pred;
            delete_rec(y, pred, t);
        } else if (z->n >= t) { /* 情況 2b：右小孩夠多 → 用後繼取代 */
            BTNode *s = z;
            while (!s->leaf)
                s = s->child[0];
            int succ = s->keys[0];
            x->keys[i] = succ;
            delete_rec(z, succ, t);
        } else { /* 情況 2c：兩邊都只有 t-1 個 → 合併成一個 2t-1 的節點，再往下刪 */
            merge(x, i);
            delete_rec(y, key, t);
        }
        return;
    }
    if (x->leaf)
        return; /* 不存在（呼叫前已確認存在，不會走到這裡） */
    BTNode *c = x->child[i];
    if (c->n == t - 1) { /* 情況 3：要往下走的小孩只有 t-1 個，先補到 t 個 */
        BTNode *left = i > 0 ? x->child[i - 1] : NULL, *right = i < x->n ? x->child[i + 1] : NULL;
        if (left && left->n >= t) { /* 3a：向左兄弟借（透過父節點轉一個過來） */
            for (int j = c->n - 1; j >= 0; j--)
                c->keys[j + 1] = c->keys[j];
            if (!c->leaf)
                for (int j = c->n; j >= 0; j--)
                    c->child[j + 1] = c->child[j];
            c->keys[0] = x->keys[i - 1];
            if (!c->leaf)
                c->child[0] = left->child[left->n];
            x->keys[i - 1] = left->keys[left->n - 1];
            left->n--;
            c->n++;
        } else if (right && right->n >= t) { /* 3a：向右兄弟借 */
            c->keys[c->n] = x->keys[i];
            if (!c->leaf)
                c->child[c->n + 1] = right->child[0];
            x->keys[i] = right->keys[0];
            for (int j = 0; j < right->n - 1; j++)
                right->keys[j] = right->keys[j + 1];
            if (!right->leaf)
                for (int j = 0; j < right->n; j++)
                    right->child[j] = right->child[j + 1];
            right->n--;
            c->n++;
        } else if (right) { /* 3b：兄弟都不夠 → 跟右兄弟合併 */
            merge(x, i);
        } else { /* 3b：沒有右兄弟 → 跟左兄弟合併 */
            merge(x, i - 1);
            c = left;
        }
    }
    delete_rec(c, key, t);
}

int bt_delete(BTree *b, int key)
{
    if (!bt_search(b, key))
        return 0;
    delete_rec(b->root, key, b->t);
    if (b->root->n == 0 && !b->root->leaf) { /* 根被合併光了：樹變矮一層 */
        BTNode *old = b->root;
        b->root = old->child[0];
        node_free(old);
    }
    b->size--;
    return 1;
}

static void inorder_rec(const BTNode *x, int *out, int *k)
{
    for (int i = 0; i < x->n; i++) {
        if (!x->leaf)
            inorder_rec(x->child[i], out, k);
        out[(*k)++] = x->keys[i];
    }
    if (!x->leaf)
        inorder_rec(x->child[x->n], out, k);
}

int bt_inorder(const BTree *b, int *out)
{
    int k = 0;
    inorder_rec(b->root, out, &k);
    return k;
}

int bt_height(const BTree *b)
{
    int h = 1;
    for (const BTNode *x = b->root; !x->leaf; x = x->child[0])
        h++;
    return h;
}

/* 回傳這棵子樹的高度（葉子 = 1）；不合法回傳 -1 */
static int check_rec(const BTNode *x, int t, int is_root, long long lo, long long hi, int *count)
{
    if (x->n > 2 * t - 1 || (!is_root && x->n < t - 1))
        return -1;
    for (int i = 0; i < x->n; i++) {
        if (x->keys[i] <= lo || x->keys[i] >= hi)
            return -1;
        if (i > 0 && x->keys[i - 1] >= x->keys[i])
            return -1;
    }
    *count += x->n;
    if (x->leaf)
        return 1;
    int h = -1;
    for (int i = 0; i <= x->n; i++) {
        long long l = i == 0 ? lo : x->keys[i - 1], r = i == x->n ? hi : x->keys[i];
        int hc = check_rec(x->child[i], t, 0, l, r, count);
        if (hc < 0 || (h >= 0 && hc != h))
            return -1; /* 所有葉子必須在同一層 */
        h = hc;
    }
    return h + 1;
}

int bt_check(const BTree *b)
{
    int count = 0;
    return check_rec(b->root, b->t, 1, (long long)INT_MIN - 1, (long long)INT_MAX + 1, &count) > 0 &&
           count == b->size;
}
