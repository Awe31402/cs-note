/* LeetCode 1382 · Balance a Binary Search Tree
 * 思路：中序走訪拿到排好的節點 → 用 108 的方法重建。重用原本的節點，不重新 malloc。O(n)。
 *       （也可以把節點一個一個插進 AVL 樹，但那是 O(n log n)，而且麻煩很多。）
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
static void collect(struct TreeNode *t, struct TreeNode **arr, int *k)
{
    if (!t)
        return;
    collect(t->left, arr, k);
    arr[(*k)++] = t;
    collect(t->right, arr, k);
}

static struct TreeNode *rebuild(struct TreeNode **arr, int lo, int hi)
{
    if (lo >= hi)
        return NULL;
    int mid = lo + (hi - lo) / 2;
    struct TreeNode *root = arr[mid];
    root->left = rebuild(arr, lo, mid);
    root->right = rebuild(arr, mid + 1, hi);
    return root;
}

struct TreeNode *balanceBST(struct TreeNode *root)
{
    struct TreeNode **arr = malloc(10000 * sizeof *arr); /* 題目：最多 10^4 個節點 */
    int n = 0;
    collect(root, arr, &n);
    root = rebuild(arr, 0, n);
    free(arr);
    return root;
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
    /* 已排序的插入 → 一條直線 */
    struct TreeNode *r = mk(1), *p = r;
    for (int v = 2; v <= 1000; v++) {
        p->right = mk(v);
        p = p->right;
    }
    assert(height(r) == 1000);
    r = balanceBST(r);
    static int out[1000];
    assert(inorder(r, out, 0) == 1000);
    for (int i = 0; i < 1000; i++)
        assert(out[i] == i + 1);
    assert(balanced_slow(r) && height(r) == 10);
    free_tree(r);
    puts("1382: passed");
    return 0;
}
