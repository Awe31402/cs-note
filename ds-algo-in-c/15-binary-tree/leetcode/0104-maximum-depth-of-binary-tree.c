/* LeetCode 104 · Maximum Depth of Binary Tree
 * 思路：遞迴。深度 = 1 + max(左子樹深度, 右子樹深度)，空樹是 0。
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
int maxDepth(struct TreeNode *root)
{
    if (!root)
        return 0;
    int l = maxDepth(root->left), r = maxDepth(root->right);
    return 1 + (l > r ? l : r);
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

int main(void)
{
    int a[] = {3, 9, 20, NUL, NUL, 15, 7};
    struct TreeNode *t = build(a, 7);
    assert(maxDepth(t) == 3);
    free_tree(t);
    int b[] = {1, NUL, 2};
    t = build(b, 3);
    assert(maxDepth(t) == 2);
    free_tree(t);
    assert(maxDepth(NULL) == 0);
    puts("0104: passed");
    return 0;
}
