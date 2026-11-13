#include "rbtree.h"

#include <limits.h>
#include <stdlib.h>

void rb_init(RBTree *t)
{
    t->nil.color = BLACK; /* 性質 3 */
    t->nil.left = t->nil.right = t->nil.parent = &t->nil;
    t->nil.key = 0;
    t->root = &t->nil;
    t->size = 0;
}

static void free_rec(RBTree *t, RBNode *x)
{
    if (x == &t->nil)
        return;
    free_rec(t, x->left);
    free_rec(t, x->right);
    free(x);
}

void rb_free(RBTree *t)
{
    free_rec(t, t->root);
    rb_init(t);
}

/* LEFT-ROTATE(x)：x 的右小孩 y 升上來
 *      x                y
 *     / \              / \
 *    a   y     →      x   c
 *       / \          / \
 *      b   c        a   b      */
static void left_rotate(RBTree *t, RBNode *x)
{
    RBNode *y = x->right;
    x->right = y->left;
    if (y->left != &t->nil)
        y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == &t->nil)
        t->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

static void right_rotate(RBTree *t, RBNode *x)
{
    RBNode *y = x->left;
    x->left = y->right;
    if (y->right != &t->nil)
        y->right->parent = x;
    y->parent = x->parent;
    if (x->parent == &t->nil)
        t->root = y;
    else if (x == x->parent->right)
        x->parent->right = y;
    else
        x->parent->left = y;
    y->right = x;
    x->parent = y;
}

/* 新節點是紅的，唯一可能被破壞的是性質 4（父節點也是紅）或性質 2（新節點是根） */
static void insert_fixup(RBTree *t, RBNode *z)
{
    while (z->parent->color == RED) {
        RBNode *p = z->parent, *g = p->parent; /* 父是紅的 → 父不是根 → 祖父存在 */
        if (p == g->left) {
            RBNode *y = g->right; /* 叔叔 (uncle) */
            if (y->color == RED) {
                /* 情況 1：叔叔紅 → 父、叔變黑，祖父變紅，問題往上丟兩層 */
                p->color = BLACK;
                y->color = BLACK;
                g->color = RED;
                z = g;
            } else {
                if (z == p->right) {
                    /* 情況 2：z 是右小孩（折線）→ 左旋變成直線，轉成情況 3 */
                    z = p;
                    left_rotate(t, z);
                    p = z->parent;
                }
                /* 情況 3：z 是左小孩（直線）→ 父變黑、祖父變紅、祖父右旋。結束 */
                p->color = BLACK;
                g->color = RED;
                right_rotate(t, g);
            }
        } else { /* 左右對調 */
            RBNode *y = g->left;
            if (y->color == RED) {
                p->color = BLACK;
                y->color = BLACK;
                g->color = RED;
                z = g;
            } else {
                if (z == p->left) {
                    z = p;
                    right_rotate(t, z);
                    p = z->parent;
                }
                p->color = BLACK;
                g->color = RED;
                left_rotate(t, g);
            }
        }
    }
    t->root->color = BLACK; /* 性質 2 */
}

int rb_insert(RBTree *t, int key)
{
    RBNode *y = &t->nil, *x = t->root;
    while (x != &t->nil) {
        y = x;
        if (key == x->key)
            return 0;
        x = key < x->key ? x->left : x->right;
    }
    RBNode *z = malloc(sizeof *z);
    if (!z)
        abort();
    z->key = key;
    z->parent = y;
    if (y == &t->nil)
        t->root = z;
    else if (key < y->key)
        y->left = z;
    else
        y->right = z;
    z->left = z->right = &t->nil;
    z->color = RED; /* 紅的不會破壞性質 5 */
    t->size++;
    insert_fixup(t, z);
    return 1;
}

static RBNode *find(const RBTree *t, int key)
{
    RBNode *x = t->root;
    while (x != &t->nil && x->key != key)
        x = key < x->key ? x->left : x->right;
    return x;
}

RBNode *rb_search(const RBTree *t, int key)
{
    RBNode *x = find(t, key);
    return x == &t->nil ? NULL : x;
}

RBNode *rb_lower_bound(const RBTree *t, int key)
{
    RBNode *x = t->root, *best = NULL;
    while (x != &t->nil) {
        if (x->key >= key) {
            best = x;
            x = x->left;
        } else {
            x = x->right;
        }
    }
    return best;
}

/* RB-TRANSPLANT：跟 BST 版不同的是 v 可能是 nil，也照樣設 parent（delete_fixup 需要 x->parent） */
static void transplant(RBTree *t, RBNode *u, RBNode *v)
{
    if (u->parent == &t->nil)
        t->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    v->parent = u->parent;
}

/* x 多背了一層「額外的黑」。把它往上推，或用旋轉＋變色把它消化掉。 */
static void delete_fixup(RBTree *t, RBNode *x)
{
    while (x != t->root && x->color == BLACK) {
        if (x == x->parent->left) {
            RBNode *w = x->parent->right; /* 兄弟 (sibling) */
            if (w->color == RED) {
                /* 情況 1：兄弟紅 → 兄弟變黑、父變紅、父左旋，轉成兄弟黑的情況 */
                w->color = BLACK;
                x->parent->color = RED;
                left_rotate(t, x->parent);
                w = x->parent->right;
            }
            if (w->left->color == BLACK && w->right->color == BLACK) {
                /* 情況 2：兄弟黑、兄弟兩個小孩都黑 → 兄弟變紅，額外的黑往上推給父 */
                w->color = RED;
                x = x->parent;
            } else {
                if (w->right->color == BLACK) {
                    /* 情況 3：兄弟黑、兄弟左小孩紅右小孩黑 → 兄弟右旋，轉成情況 4 */
                    w->left->color = BLACK;
                    w->color = RED;
                    right_rotate(t, w);
                    w = x->parent->right;
                }
                /* 情況 4：兄弟黑、兄弟右小孩紅 → 變色 + 父左旋，額外的黑被消化。結束 */
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                left_rotate(t, x->parent);
                x = t->root;
            }
        } else { /* 左右對調 */
            RBNode *w = x->parent->left;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                right_rotate(t, x->parent);
                w = x->parent->left;
            }
            if (w->right->color == BLACK && w->left->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color = RED;
                    left_rotate(t, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                right_rotate(t, x->parent);
                x = t->root;
            }
        }
    }
    x->color = BLACK; /* x 是「紅＋額外的黑」：直接塗黑就好 */
}

int rb_delete(RBTree *t, int key)
{
    RBNode *z = find(t, key);
    if (z == &t->nil)
        return 0;
    RBNode *y = z, *x;
    Color y_orig = y->color; /* 真正從樹上「消失」的那個位置原本的顏色 */
    if (z->left == &t->nil) {
        x = z->right;
        transplant(t, z, z->right);
    } else if (z->right == &t->nil) {
        x = z->left;
        transplant(t, z, z->left);
    } else {
        y = z->right; /* 後繼 */
        while (y->left != &t->nil)
            y = y->left;
        y_orig = y->color;
        x = y->right;
        if (y != z->right) {
            transplant(t, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        } else {
            x->parent = y; /* x 可能是 nil，也要設好 parent */
        }
        transplant(t, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color; /* y 接手 z 的位置和顏色 */
    }
    free(z);
    t->size--;
    if (y_orig == BLACK) /* 少了一個黑節點，性質 5 被破壞 */
        delete_fixup(t, x);
    return 1;
}

static void inorder_rec(const RBTree *t, const RBNode *x, int *out, int *k)
{
    if (x == &t->nil)
        return;
    inorder_rec(t, x->left, out, k);
    out[(*k)++] = x->key;
    inorder_rec(t, x->right, out, k);
}

int rb_inorder(const RBTree *t, int *out)
{
    int k = 0;
    inorder_rec(t, t->root, out, &k);
    return k;
}

static int height_rec(const RBTree *t, const RBNode *x)
{
    if (x == &t->nil)
        return 0;
    int l = height_rec(t, x->left), r = height_rec(t, x->right);
    return 1 + (l > r ? l : r);
}

int rb_height(const RBTree *t)
{
    return height_rec(t, t->root);
}

/* 回傳 black-height（不含自己之上的），不合法回傳 -1 */
static int check_rec(const RBTree *t, const RBNode *x, const RBNode *parent, long long lo, long long hi, int *count)
{
    if (x == &t->nil)
        return 1; /* NIL 算一個黑 */
    (*count)++;
    if (x->parent != parent || x->key <= lo || x->key >= hi)
        return -1;
    if (x->color == RED && (x->left->color == RED || x->right->color == RED))
        return -1; /* 性質 4 */
    int l = check_rec(t, x->left, x, lo, x->key, count);
    int r = check_rec(t, x->right, x, x->key, hi, count);
    if (l < 0 || r < 0 || l != r)
        return -1; /* 性質 5 */
    return l + (x->color == BLACK);
}

int rb_check(const RBTree *t)
{
    if (t->root->color != BLACK || t->nil.color != BLACK) /* 性質 2、3 */
        return -1;
    int count = 0;
    int bh = check_rec(t, t->root, &t->nil, (long long)INT_MIN - 1, (long long)INT_MAX + 1, &count);
    return count == t->size ? bh : -1;
}
