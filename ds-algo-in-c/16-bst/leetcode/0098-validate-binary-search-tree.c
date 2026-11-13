/* LeetCode 98 · Validate Binary Search Tree
 * 思路：每個節點都有一個合法範圍 (lo, hi)。往左走時上界變成自己，往右走時下界變成自己。
 *       只跟父節點比是錯的：右子樹裡的每一個都要大於「祖先」，不只是父節點。
 */
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct TreeNode {
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
};

/* ===== 提交範圍 開始 ===== */
static bool valid(const struct TreeNode *t, long long lo, long long hi)
{
    if (!t)
        return true;
    if (t->val <= lo || t->val >= hi)
        return false;
    return valid(t->left, lo, t->val) && valid(t->right, t->val, hi);
}

bool isValidBST(struct TreeNode *root)
{
    /* 用 long long 當邊界：節點值可能剛好是 INT_MIN / INT_MAX */
    return valid(root, (long long)INT_MIN - 1, (long long)INT_MAX + 1);
}
/* ===== 提交範圍 結束 ===== */

static struct TreeNode *insert(struct TreeNode *r, int v) /* 本機測試用：普通 BST 插入 */
{
    if (!r) {
        r = malloc(sizeof *r);
        r->val = v;
        r->left = r->right = NULL;
        return r;
    }
    if (v < r->val)
        r->left = insert(r->left, v);
    else if (v > r->val)
        r->right = insert(r->right, v);
    return r;
}

static void free_tree(struct TreeNode *r)
{
    if (!r)
        return;
    free_tree(r->left);
    free_tree(r->right);
    free(r);
}

static struct TreeNode *mk(int v, struct TreeNode *l, struct TreeNode *r)
{
    struct TreeNode *n = malloc(sizeof *n);
    n->val = v;
    n->left = l;
    n->right = r;
    return n;
}

int main(void)
{
    struct TreeNode *a = mk(2, mk(1, NULL, NULL), mk(3, NULL, NULL));
    assert(isValidBST(a));
    free_tree(a);
    /* 5 的右子樹裡有 3 < 5：只跟父節點比會誤判成合法 */
    struct TreeNode *b = mk(5, mk(1, NULL, NULL), mk(6, mk(3, NULL, NULL), mk(7, NULL, NULL)));
    assert(!isValidBST(b));
    free_tree(b);
    struct TreeNode *c = mk(2, mk(2, NULL, NULL), NULL); /* 相等也不行 */
    assert(!isValidBST(c));
    free_tree(c);
    struct TreeNode *d = mk(INT_MAX, mk(INT_MIN, NULL, NULL), NULL);
    assert(isValidBST(d));
    free_tree(d);
    struct TreeNode *e = NULL;
    for (int i = 0; i < 50; i++)
        e = insert(e, (i * 37) % 101);
    assert(isValidBST(e));
    free_tree(e);
    puts("0098: passed");
    return 0;
}
