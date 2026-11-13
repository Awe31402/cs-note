/* LeetCode 105 · Construct Binary Tree from Preorder and Inorder Traversal
 * 思路：前序第一個是根；在中序裡找到根，左邊是左子樹、右邊是右子樹，遞迴。
 *       值的範圍 -3000..3000，用陣列記「值 → 中序位置」，找根變 O(1)，整體 O(n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct TreeNode {
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
};

/* ===== 提交範圍 開始 ===== */
static int pos[6001]; /* pos[v + 3000] = v 在中序的 index */
static int pre_i;     /* 前序目前讀到哪 */

static struct TreeNode *build_rec(const int *pre, int lo, int hi) /* 中序區間 [lo, hi) */
{
    if (lo >= hi)
        return NULL;
    struct TreeNode *root = malloc(sizeof *root);
    root->val = pre[pre_i++];
    int m = pos[root->val + 3000];
    root->left = build_rec(pre, lo, m); /* 前序是「根 左 右」，所以一定先建左邊 */
    root->right = build_rec(pre, m + 1, hi);
    return root;
}

struct TreeNode *buildTree(int *preorder, int preorderSize, int *inorder, int inorderSize)
{
    for (int i = 0; i < inorderSize; i++)
        pos[inorder[i] + 3000] = i;
    pre_i = 0;
    return build_rec(preorder, 0, preorderSize);
}
/* ===== 提交範圍 結束 ===== */

#define NUL (-1000000) /* 本機測試用的 null 標記 */

static struct TreeNode *node(int v)
{
    struct TreeNode *n = malloc(sizeof *n);
    n->val = v;
    n->left = n->right = NULL;
    return n;
}

/* LeetCode 層序格式 → 樹 */
static struct TreeNode *build(const int *a, int n)
{
    if (n == 0 || a[0] == NUL)
        return NULL;
    struct TreeNode **q = malloc((size_t)n * sizeof *q), *root = node(a[0]);
    int h = 0, t = 0, i = 1;
    q[t++] = root;
    while (h < t && i < n) {
        struct TreeNode *p = q[h++];
        if (i < n && a[i] != NUL)
            q[t++] = p->left = node(a[i]);
        i++;
        if (i < n && a[i] != NUL)
            q[t++] = p->right = node(a[i]);
        i++;
    }
    free(q);
    return root;
}

static void free_tree(struct TreeNode *r)
{
    if (!r)
        return;
    free_tree(r->left);
    free_tree(r->right);
    free(r);
}

static void walk(struct TreeNode *t, int *pre, int *in, int *a, int *b)
{
    if (!t)
        return;
    pre[(*a)++] = t->val;
    walk(t->left, pre, in, a, b);
    in[(*b)++] = t->val;
    walk(t->right, pre, in, a, b);
}

static int same(const struct TreeNode *a, const struct TreeNode *b)
{
    if (!a || !b)
        return a == b;
    return a->val == b->val && same(a->left, b->left) && same(a->right, b->right);
}

int main(void)
{
    int pre[] = {3, 9, 20, 15, 7}, in[] = {9, 3, 15, 20, 7};
    int lv[] = {3, 9, 20, NUL, NUL, 15, 7};
    struct TreeNode *t = buildTree(pre, 5, in, 5), *want = build(lv, 7);
    assert(same(t, want));
    free_tree(t);
    free_tree(want);

    /* 隨機樹：走訪 → 重建 → 再走訪要一樣 */
    srand(105);
    for (int k = 0; k < 300; k++) {
        int n = 1 + rand() % 40;
        struct TreeNode *r = node(-20); /* 值 = v - 20，v = 0..n-1，保證不重複 */
        for (int v = 1; v < n; v++) {
            struct TreeNode *p = r;
            for (;;) {
                struct TreeNode **c = rand() % 2 ? &p->left : &p->right;
                if (!*c) {
                    *c = node(v - 20); /* 有負數 */
                    break;
                }
                p = *c;
            }
        }
        int p1[40], i1[40], p2[40], i2[40], a = 0, b = 0;
        walk(r, p1, i1, &a, &b);
        struct TreeNode *c = buildTree(p1, n, i1, n);
        a = b = 0;
        walk(c, p2, i2, &a, &b);
        for (int i = 0; i < n; i++)
            assert(p1[i] == p2[i] && i1[i] == i2[i]);
        free_tree(r);
        free_tree(c);
    }
    puts("0105: passed");
    return 0;
}
