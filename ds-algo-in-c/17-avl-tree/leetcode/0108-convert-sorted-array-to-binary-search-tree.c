/* LeetCode 108 · Convert Sorted Array to Binary Search Tree（要高度平衡）
 * 思路：中間的元素當根，左半邊遞迴建左子樹、右半邊遞迴建右子樹。
 *       左右兩半的大小最多差 1，所以一定平衡。O(n)。
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
static struct TreeNode *build(const int *a, int lo, int hi) /* [lo, hi) */
{
    if (lo >= hi)
        return NULL;
    int mid = lo + (hi - lo) / 2;
    struct TreeNode *n = malloc(sizeof *n);
    n->val = a[mid];
    n->left = build(a, lo, mid);
    n->right = build(a, mid + 1, hi);
    return n;
}

struct TreeNode *sortedArrayToBST(int *nums, int numsSize)
{
    return build(nums, 0, numsSize);
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
    for (int n = 1; n <= 300; n++) {
        int a[300], out[300];
        for (int i = 0; i < n; i++)
            a[i] = i * 3 - 100;
        struct TreeNode *r = sortedArrayToBST(a, n);
        assert(inorder(r, out, 0) == n); /* 中序還原成原陣列 = 合法 BST */
        for (int i = 0; i < n; i++)
            assert(out[i] == a[i]);
        assert(balanced_slow(r));
        int hmin = 0;
        while ((1 << hmin) - 1 < n)
            hmin++;
        assert(height(r) == hmin); /* 最矮的可能高度 */
        free_tree(r);
    }
    (void)mk;
    puts("0108: passed");
    return 0;
}
