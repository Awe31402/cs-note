/* LeetCode 226 · Invert Binary Tree（鏡像）
 * 思路：遞迴。交換左右小孩，再各自翻轉。前序、後序都可以。
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
struct TreeNode *invertTree(struct TreeNode *root)
{
    if (!root)
        return NULL;
    struct TreeNode *t = root->left;
    root->left = invertTree(root->right);
    root->right = invertTree(t);
    return root;
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

static int level(struct TreeNode *root, int *out)
{
    struct TreeNode *q[64];
    int h = 0, t = 0, k = 0;
    if (root)
        q[t++] = root;
    while (h < t) {
        struct TreeNode *p = q[h++];
        out[k++] = p->val;
        if (p->left)
            q[t++] = p->left;
        if (p->right)
            q[t++] = p->right;
    }
    return k;
}

int main(void)
{
    int a[] = {4, 2, 7, 1, 3, 6, 9}, wa[] = {4, 7, 2, 9, 6, 3, 1}, out[16];
    struct TreeNode *t = invertTree(build(a, 7));
    assert(level(t, out) == 7);
    for (int i = 0; i < 7; i++)
        assert(out[i] == wa[i]);
    free_tree(t);
    int b[] = {2, 1, 3}, wb[] = {2, 3, 1};
    t = invertTree(build(b, 3));
    assert(level(t, out) == 3);
    for (int i = 0; i < 3; i++)
        assert(out[i] == wb[i]);
    free_tree(t);
    assert(invertTree(NULL) == NULL);
    puts("0226: passed");
    return 0;
}
