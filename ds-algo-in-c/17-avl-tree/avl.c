#include "avl.h"

#include <limits.h>
#include <stdlib.h>

static int h(const AVLNode *x) { return x ? x->height : 0; }
static int max(int a, int b) { return a > b ? a : b; }
static void update(AVLNode *x) { x->height = 1 + max(h(x->left), h(x->right)); }
static int balance(const AVLNode *x) { return h(x->left) - h(x->right); }

/* 右旋：左小孩 y 升上來當根，x 變成 y 的右小孩，y 原本的右子樹 B 過繼給 x 當左子樹
 *        x              y
 *       / \            / \
 *      y   C   →      A   x
 *     / \                / \
 *    A   B              B   C        中序 A y B x C 不變 */
static AVLNode *rotate_right(AVL *t, AVLNode *x)
{
    AVLNode *y = x->left;
    x->left = y->right;
    y->right = x;
    update(x); /* x 在下面，先更新 */
    update(y);
    t->rotations++;
    return y;
}

static AVLNode *rotate_left(AVL *t, AVLNode *x)
{
    AVLNode *y = x->right;
    x->right = y->left;
    y->left = x;
    update(x);
    update(y);
    t->rotations++;
    return y;
}

/* 四種失衡：LL、RR 一次旋轉；LR、RL 兩次旋轉 */
static AVLNode *rebalance(AVL *t, AVLNode *x)
{
    update(x);
    int b = balance(x);
    if (b > 1) {                    /* 左邊太高 */
        if (balance(x->left) < 0)   /* LR：左小孩是「右邊比較高」 */
            x->left = rotate_left(t, x->left);
        return rotate_right(t, x);  /* LL */
    }
    if (b < -1) {                   /* 右邊太高 */
        if (balance(x->right) > 0)  /* RL */
            x->right = rotate_right(t, x->right);
        return rotate_left(t, x);   /* RR */
    }
    return x;
}

void avl_init(AVL *t)
{
    t->root = NULL;
    t->size = 0;
    t->rotations = 0;
}

static void free_rec(AVLNode *x)
{
    if (!x)
        return;
    free_rec(x->left);
    free_rec(x->right);
    free(x);
}

void avl_free(AVL *t)
{
    free_rec(t->root);
    avl_init(t);
}

static AVLNode *insert_rec(AVL *t, AVLNode *x, int key, int *added)
{
    if (!x) {
        AVLNode *n = malloc(sizeof *n);
        if (!n)
            abort();
        n->key = key;
        n->height = 1;
        n->left = n->right = NULL;
        *added = 1;
        return n;
    }
    if (key < x->key)
        x->left = insert_rec(t, x->left, key, added);
    else if (key > x->key)
        x->right = insert_rec(t, x->right, key, added);
    else
        return x; /* 已存在 */
    return rebalance(t, x); /* 遞迴回來的路上，每一層都檢查一次 */
}

int avl_insert(AVL *t, int key)
{
    int added = 0;
    t->root = insert_rec(t, t->root, key, &added);
    t->size += added;
    return added;
}

static AVLNode *delete_rec(AVL *t, AVLNode *x, int key, int *removed)
{
    if (!x)
        return NULL;
    if (key < x->key) {
        x->left = delete_rec(t, x->left, key, removed);
    } else if (key > x->key) {
        x->right = delete_rec(t, x->right, key, removed);
    } else {
        *removed = 1;
        if (!x->left || !x->right) {
            AVLNode *child = x->left ? x->left : x->right;
            free(x);
            return child; /* 小孩本身已經平衡 */
        }
        AVLNode *s = x->right; /* 兩個小孩：後繼的 key 搬上來，再到右子樹刪後繼 */
        while (s->left)
            s = s->left;
        x->key = s->key;
        int dummy = 0;
        x->right = delete_rec(t, x->right, s->key, &dummy);
    }
    return rebalance(t, x); /* 刪除可能讓一路上好幾層都要旋轉 */
}

int avl_delete(AVL *t, int key)
{
    int removed = 0;
    t->root = delete_rec(t, t->root, key, &removed);
    t->size -= removed;
    return removed;
}

int avl_contains(const AVL *t, int key)
{
    const AVLNode *x = t->root;
    while (x && x->key != key)
        x = key < x->key ? x->left : x->right;
    return x != NULL;
}

static void inorder_rec(const AVLNode *x, int *out, int *k)
{
    if (!x)
        return;
    inorder_rec(x->left, out, k);
    out[(*k)++] = x->key;
    inorder_rec(x->right, out, k);
}

int avl_inorder(const AVL *t, int *out)
{
    int k = 0;
    inorder_rec(t->root, out, &k);
    return k;
}

int avl_height(const AVL *t)
{
    return h(t->root);
}

/* 回傳子樹實際高度；不合法回傳 -1 */
static int check_rec(const AVLNode *x, long long lo, long long hi, int *count)
{
    if (!x)
        return 0;
    (*count)++;
    if (x->key <= lo || x->key >= hi)
        return -1;
    int l = check_rec(x->left, lo, x->key, count);
    int r = check_rec(x->right, x->key, hi, count);
    if (l < 0 || r < 0 || l - r > 1 || r - l > 1 || x->height != 1 + max(l, r))
        return -1;
    return x->height;
}

int avl_check(const AVL *t)
{
    int count = 0;
    return check_rec(t->root, (long long)INT_MIN - 1, (long long)INT_MAX + 1, &count) >= 0 &&
           count == t->size;
}
