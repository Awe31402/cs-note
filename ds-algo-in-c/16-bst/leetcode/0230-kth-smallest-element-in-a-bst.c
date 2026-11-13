/* LeetCode 230 · Kth Smallest Element in a BST
 * 思路：BST 的中序走訪是遞增的，走到第 k 個就停。迴圈版中序，O(h + k)。
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
int kthSmallest(struct TreeNode *root, int k)
{
    struct TreeNode *st[10000]; /* 題目：最多 10^4 個節點 */
    int top = 0;
    struct TreeNode *cur = root;
    for (;;) {
        while (cur) {
            st[top++] = cur;
            cur = cur->left;
        }
        cur = st[--top];
        if (--k == 0)
            return cur->val;
        cur = cur->right;
    }
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
    int a[] = {3, 1, 4, 2};
    for (int i = 0; i < 4; i++)
        t = insert(t, a[i]);
    for (int k = 1; k <= 4; k++)
        assert(kthSmallest(t, k) == k);
    free_tree(t);
    t = NULL;
    for (int i = 0; i < 1000; i++)
        t = insert(t, (i * 7919) % 1000); /* 0..999 打亂插入 */
    for (int k = 1; k <= 1000; k += 37)
        assert(kthSmallest(t, k) == k - 1);
    free_tree(t);
    puts("0230: passed");
    return 0;
}
