/* LeetCode 110 · Balanced Binary Tree
 * 思路：後序遞迴，一次算出「高度」並檢查平衡。不平衡就回傳 -1 一路往上傳。O(n)。
 *       每個節點都各算一次左右高度的直覺寫法是 O(n²)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct TreeNode {
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
};

/* ===== 提交範圍 開始 ===== */
static int check(struct TreeNode *t) /* 平衡就回傳高度，不平衡回傳 -1 */
{
    if (!t)
        return 0;
    int l = check(t->left);
    if (l < 0)
        return -1;
    int r = check(t->right);
    if (r < 0 || l - r > 1 || r - l > 1)
        return -1;
    return 1 + (l > r ? l : r);
}

bool isBalanced(struct TreeNode *root)
{
    return check(root) >= 0;
}
/* ===== 提交範圍 結束 ===== */

static struct TreeNode *mk(int v)
{
    struct TreeNode *n = malloc(sizeof *n);
    n->val = v;
    n->left = n->right = NULL;
    return n;
}

static void free_tree(struct TreeNode *r)
{
    if (!r)
        return;
    free_tree(r->left);
    free_tree(r->right);
    free(r);
}

static int height(const struct TreeNode *r)
{
    if (!r)
        return 0;
    int a = height(r->left), b = height(r->right);
    return 1 + (a > b ? a : b);
}

static int balanced_slow(const struct TreeNode *r) /* O(n^2) 的直覺版，用來對照 */
{
    if (!r)
        return 1;
    int d = height(r->left) - height(r->right);
    return d >= -1 && d <= 1 && balanced_slow(r->left) && balanced_slow(r->right);
}

static int inorder(const struct TreeNode *r, int *out, int k)
{
    if (!r)
        return k;
    k = inorder(r->left, out, k);
    out[k++] = r->val;
    return inorder(r->right, out, k);
}

int main(void)
{
    struct TreeNode *a = mk(3); /* [3,9,20,null,null,15,7] */
    a->left = mk(9);
    a->right = mk(20);
    a->right->left = mk(15);
    a->right->right = mk(7);
    assert(isBalanced(a));
    free_tree(a);

    struct TreeNode *b = mk(1); /* 左邊一路長下去 */
    b->left = mk(2);
    b->right = mk(2);
    b->left->left = mk(3);
    b->left->right = mk(3);
    b->left->left->left = mk(4);
    b->left->left->right = mk(4);
    assert(!isBalanced(b));
    free_tree(b);
    assert(isBalanced(NULL));

    /* 根的左右高度一樣，但下面某個節點不平衡：只檢查根是不夠的 */
    struct TreeNode *c = mk(1);
    c->left = mk(2);
    c->right = mk(2);
    c->left->left = mk(3);
    c->left->left->left = mk(4);
    c->right->right = mk(3);
    c->right->right->right = mk(4);
    assert(!isBalanced(c) && !balanced_slow(c));
    free_tree(c);

    srand(110);
    for (int t = 0; t < 500; t++) { /* 隨機樹：跟 O(n²) 版對照 */
        int n = 1 + rand() % 20;
        struct TreeNode *r = mk(0);
        for (int v = 1; v < n; v++) {
            struct TreeNode *p = r;
            for (;;) {
                struct TreeNode **ch = rand() % 2 ? &p->left : &p->right;
                if (!*ch) {
                    *ch = mk(v);
                    break;
                }
                p = *ch;
            }
        }
        assert(isBalanced(r) == balanced_slow(r));
        free_tree(r);
    }
    (void)inorder;
    puts("0110: passed");
    return 0;
}
