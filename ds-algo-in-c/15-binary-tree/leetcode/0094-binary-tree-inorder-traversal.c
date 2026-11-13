/* LeetCode 94 · Binary Tree Inorder Traversal（迴圈版）
 * 思路：一路往左走到底、沿路 push；pop 出來就輸出，然後轉去右子樹。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TreeNode {
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
};

/* ===== 提交範圍 開始 ===== */
int *inorderTraversal(struct TreeNode *root, int *returnSize)
{
    int *out = malloc(100 * sizeof *out); /* 題目：最多 100 個節點 */
    struct TreeNode *st[100];
    int top = 0, k = 0;
    struct TreeNode *cur = root;
    while (cur || top > 0) {
        while (cur) {
            st[top++] = cur;
            cur = cur->left;
        }
        cur = st[--top];
        out[k++] = cur->val;
        cur = cur->right;
    }
    *returnSize = k;
    return out;
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
    int a[] = {1, NUL, 2, 3}, wa[] = {1, 3, 2}, m;
    struct TreeNode *t = build(a, 4);
    int *r = inorderTraversal(t, &m);
    assert(m == 3 && memcmp(r, wa, sizeof wa) == 0);
    free(r);
    free_tree(t);

    int b[] = {1, 2, 3, 4, 5, NUL, 8, NUL, NUL, 6, 7, 9}, wb[] = {4, 2, 6, 5, 7, 1, 3, 9, 8};
    t = build(b, 12);
    r = inorderTraversal(t, &m);
    assert(m == 9 && memcmp(r, wb, sizeof wb) == 0);
    free(r);
    free_tree(t);

    r = inorderTraversal(NULL, &m);
    assert(m == 0);
    free(r);
    puts("0094: passed");
    return 0;
}
