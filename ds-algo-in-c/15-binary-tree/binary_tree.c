#include "binary_tree.h"

#include <stdlib.h>

TNode *tn_new(int val)
{
    TNode *n = malloc(sizeof *n);
    if (!n)
        abort();
    n->val = val;
    n->left = n->right = NULL;
    return n;
}

void tree_free(TNode *root)
{
    if (!root)
        return;
    tree_free(root->left); /* 後序：先釋放小孩，才能釋放自己 */
    tree_free(root->right);
    free(root);
}

TNode *tree_from_level(const int *vals, int n, int null_mark)
{
    if (n == 0 || vals[0] == null_mark)
        return NULL;
    TNode **q = malloc((size_t)n * sizeof *q);
    int head = 0, tail = 0, i = 1;
    TNode *root = tn_new(vals[0]);
    q[tail++] = root;
    while (head < tail && i < n) { /* 依序幫每個節點接上左右小孩 */
        TNode *p = q[head++];
        if (i < n && vals[i] != null_mark)
            q[tail++] = p->left = tn_new(vals[i]);
        i++;
        if (i < n && vals[i] != null_mark)
            q[tail++] = p->right = tn_new(vals[i]);
        i++;
    }
    free(q);
    return root;
}

static void pre_rec(const TNode *t, int *out, int *k)
{
    if (!t)
        return;
    out[(*k)++] = t->val;
    pre_rec(t->left, out, k);
    pre_rec(t->right, out, k);
}

static void in_rec(const TNode *t, int *out, int *k)
{
    if (!t)
        return;
    in_rec(t->left, out, k);
    out[(*k)++] = t->val;
    in_rec(t->right, out, k);
}

static void post_rec(const TNode *t, int *out, int *k)
{
    if (!t)
        return;
    post_rec(t->left, out, k);
    post_rec(t->right, out, k);
    out[(*k)++] = t->val;
}

int preorder(const TNode *root, int *out) { int k = 0; pre_rec(root, out, &k); return k; }
int inorder(const TNode *root, int *out) { int k = 0; in_rec(root, out, &k); return k; }
int postorder(const TNode *root, int *out) { int k = 0; post_rec(root, out, &k); return k; }

int tree_size(const TNode *root)
{
    return root ? 1 + tree_size(root->left) + tree_size(root->right) : 0;
}

int tree_height(const TNode *root)
{
    if (!root)
        return 0;
    int l = tree_height(root->left), r = tree_height(root->right);
    return 1 + (l > r ? l : r);
}

int preorder_iter(const TNode *root, int *out)
{
    int n = tree_size(root), top = 0, k = 0;
    const TNode **st = malloc((size_t)(n + 1) * sizeof *st);
    if (root)
        st[top++] = root;
    while (top > 0) {
        const TNode *t = st[--top];
        out[k++] = t->val;
        if (t->right) /* 先放右，後放左：左邊會先被拿出來 */
            st[top++] = t->right;
        if (t->left)
            st[top++] = t->left;
    }
    free(st);
    return k;
}

int inorder_iter(const TNode *root, int *out)
{
    int n = tree_size(root), top = 0, k = 0;
    const TNode **st = malloc((size_t)(n + 1) * sizeof *st);
    const TNode *cur = root;
    while (cur || top > 0) {
        while (cur) { /* 一路往左走到底，沿路存起來 */
            st[top++] = cur;
            cur = cur->left;
        }
        cur = st[--top];
        out[k++] = cur->val; /* 左邊處理完了，輪到自己 */
        cur = cur->right;    /* 再處理右子樹 */
    }
    free(st);
    return k;
}

int postorder_iter(const TNode *root, int *out)
{
    /* 技巧：用「根 右 左」的前序走，結果反過來就是「左 右 根」 */
    int k = 0;
    int n = tree_size(root), top = 0;
    const TNode **st = malloc((size_t)(n + 1) * sizeof *st);
    if (root)
        st[top++] = root;
    while (top > 0) {
        const TNode *t = st[--top];
        out[k++] = t->val;
        if (t->left)
            st[top++] = t->left;
        if (t->right)
            st[top++] = t->right;
    }
    for (int l = 0, r = k - 1; l < r; l++, r--) {
        int tmp = out[l];
        out[l] = out[r];
        out[r] = tmp;
    }
    free(st);
    return k;
}

int level_order(const TNode *root, int *out)
{
    int n = tree_size(root), head = 0, tail = 0, k = 0;
    const TNode **q = malloc((size_t)(n + 1) * sizeof *q);
    if (root)
        q[tail++] = root;
    while (head < tail) {
        const TNode *t = q[head++];
        out[k++] = t->val;
        if (t->left)
            q[tail++] = t->left;
        if (t->right)
            q[tail++] = t->right;
    }
    free(q);
    return k;
}

TNode *build_pre_in(const int *pre, const int *in, int n)
{
    if (n == 0)
        return NULL;
    TNode *root = tn_new(pre[0]); /* 前序第一個是根 */
    int m = 0;
    while (in[m] != pre[0])
        m++; /* 中序裡根的左邊 m 個是左子樹 */
    root->left = build_pre_in(pre + 1, in, m);
    root->right = build_pre_in(pre + 1 + m, in + m + 1, n - m - 1);
    return root;
}
