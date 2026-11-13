/* LeetCode 235 · Lowest Common Ancestor of a Binary Search Tree
 * 思路：從根往下走。p、q 都比目前小 → 往左；都比目前大 → 往右；
 *       一個在左一個在右（或其中一個就是目前節點）→ 目前節點就是 LCA。O(h)。
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
struct TreeNode *lowestCommonAncestor(struct TreeNode *root, struct TreeNode *p, struct TreeNode *q)
{
    while (root) {
        if (p->val < root->val && q->val < root->val)
            root = root->left;
        else if (p->val > root->val && q->val > root->val)
            root = root->right;
        else
            return root; /* 分岔點 */
    }
    return NULL;
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

static struct TreeNode *find(struct TreeNode *r, int v)
{
    while (r && r->val != v)
        r = v < r->val ? r->left : r->right;
    return r;
}

int main(void)
{
    struct TreeNode *t = NULL;
    int a[] = {6, 2, 8, 0, 4, 7, 9, 3, 5};
    for (int i = 0; i < 9; i++)
        t = insert(t, a[i]);
    assert(lowestCommonAncestor(t, find(t, 2), find(t, 8))->val == 6);
    assert(lowestCommonAncestor(t, find(t, 2), find(t, 4))->val == 2); /* 自己也可以是祖先 */
    assert(lowestCommonAncestor(t, find(t, 3), find(t, 5))->val == 4);
    assert(lowestCommonAncestor(t, find(t, 0), find(t, 5))->val == 2);
    assert(lowestCommonAncestor(t, find(t, 7), find(t, 7))->val == 7);
    free_tree(t);
    puts("0235: passed");
    return 0;
}
