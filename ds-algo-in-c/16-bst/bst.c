#include "bst.h"

#include <limits.h>
#include <stdlib.h>

void bst_init(BST *t)
{
    t->root = NULL;
    t->size = 0;
}

static void free_rec(BNode *x)
{
    if (!x)
        return;
    free_rec(x->left);
    free_rec(x->right);
    free(x);
}

void bst_free(BST *t)
{
    free_rec(t->root);
    bst_init(t);
}

/* CLRS TREE-INSERT：從根往下走，比較小往左、比較大往右，走到 NULL 就是新節點的位置。 */
int bst_insert(BST *t, int key)
{
    BNode *parent = NULL, *x = t->root;
    while (x) {
        parent = x;
        if (key == x->key)
            return 0;
        x = key < x->key ? x->left : x->right;
    }
    BNode *z = malloc(sizeof *z);
    if (!z)
        abort();
    z->key = key;
    z->left = z->right = NULL;
    z->parent = parent;
    if (!parent)
        t->root = z; /* 原本是空樹 */
    else if (key < parent->key)
        parent->left = z;
    else
        parent->right = z;
    t->size++;
    return 1;
}

BNode *bst_search(const BST *t, int key)
{
    BNode *x = t->root;
    while (x && x->key != key)
        x = key < x->key ? x->left : x->right;
    return x;
}

BNode *bst_min(BNode *x)
{
    while (x && x->left)
        x = x->left;
    return x;
}

BNode *bst_max(BNode *x)
{
    while (x && x->right)
        x = x->right;
    return x;
}

BNode *bst_successor(BNode *x)
{
    if (x->right)
        return bst_min(x->right); /* 有右子樹：右子樹最小的 */
    BNode *y = x->parent;         /* 沒有：往上找，直到「從左邊上來」的那個祖先 */
    while (y && x == y->right) {
        x = y;
        y = y->parent;
    }
    return y;
}

BNode *bst_predecessor(BNode *x)
{
    if (x->left)
        return bst_max(x->left);
    BNode *y = x->parent;
    while (y && x == y->left) {
        x = y;
        y = y->parent;
    }
    return y;
}

BNode *bst_floor(const BST *t, int key)
{
    BNode *x = t->root, *best = NULL;
    while (x) {
        if (x->key == key)
            return x;
        if (x->key < key) {
            best = x; /* 候選，再往右找更大但仍 <= key 的 */
            x = x->right;
        } else {
            x = x->left;
        }
    }
    return best;
}

BNode *bst_ceil(const BST *t, int key)
{
    BNode *x = t->root, *best = NULL;
    while (x) {
        if (x->key == key)
            return x;
        if (x->key > key) {
            best = x;
            x = x->left;
        } else {
            x = x->right;
        }
    }
    return best;
}

/* CLRS TRANSPLANT：用子樹 v 取代子樹 u 在父節點底下的位置 */
static void transplant(BST *t, BNode *u, BNode *v)
{
    if (!u->parent)
        t->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    if (v)
        v->parent = u->parent;
}

/* CLRS TREE-DELETE 的三種情況：
 *   1. 沒有左小孩 → 右小孩頂上來（也涵蓋沒有小孩）
 *   2. 沒有右小孩 → 左小孩頂上來
 *   3. 兩個小孩都有 → 用後繼 y（右子樹最小，一定沒有左小孩）取代 z */
int bst_delete(BST *t, int key)
{
    BNode *z = bst_search(t, key);
    if (!z)
        return 0;
    if (!z->left) {
        transplant(t, z, z->right);
    } else if (!z->right) {
        transplant(t, z, z->left);
    } else {
        BNode *y = bst_min(z->right);
        if (y->parent != z) { /* y 不是 z 的直接右小孩：先把 y 從原位拿出來 */
            transplant(t, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        transplant(t, z, y);
        y->left = z->left;
        y->left->parent = y;
    }
    free(z);
    t->size--;
    return 1;
}

static void inorder_rec(const BNode *x, int *out, int *k)
{
    if (!x)
        return;
    inorder_rec(x->left, out, k);
    out[(*k)++] = x->key;
    inorder_rec(x->right, out, k);
}

int bst_inorder(const BST *t, int *out)
{
    int k = 0;
    inorder_rec(t->root, out, &k);
    return k;
}

static int height_rec(const BNode *x)
{
    if (!x)
        return 0;
    int l = height_rec(x->left), r = height_rec(x->right);
    return 1 + (l > r ? l : r);
}

int bst_height(const BST *t)
{
    return height_rec(t->root);
}

/* 每個節點的 key 必須落在 (lo, hi) 之間 —— 只跟父節點比是不夠的 */
static int check_rec(const BNode *x, const BNode *parent, long long lo, long long hi, int *count)
{
    if (!x)
        return 1;
    (*count)++;
    return x->parent == parent && lo < x->key && x->key < hi &&
           check_rec(x->left, x, lo, x->key, count) && check_rec(x->right, x, x->key, hi, count);
}

int bst_check(const BST *t)
{
    int count = 0;
    return check_rec(t->root, NULL, (long long)INT_MIN - 1, (long long)INT_MAX + 1, &count) &&
           count == t->size;
}
