/* LeetCode 700 · Search in a Binary Search Tree
 * 思路：比根小往左、比根大往右。迴圈版 O(h)，不用堆疊。
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
struct TreeNode *searchBST(struct TreeNode *root, int val)
{
    while (root && root->val != val)
        root = val < root->val ? root->left : root->right;
    return root;
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

int main(void)
{
    struct TreeNode *t = NULL;
    int a[] = {4, 2, 7, 1, 3};
    for (int i = 0; i < 5; i++)
        t = insert(t, a[i]);
    struct TreeNode *r = searchBST(t, 2);
    assert(r && r->val == 2 && r->left->val == 1 && r->right->val == 3);
    assert(searchBST(t, 5) == NULL);
    assert(searchBST(NULL, 1) == NULL);
    free_tree(t);
    puts("0700: passed");
    return 0;
}
