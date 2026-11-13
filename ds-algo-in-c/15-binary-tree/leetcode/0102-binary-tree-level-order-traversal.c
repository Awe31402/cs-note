/* LeetCode 102 · Binary Tree Level Order Traversal
 * 思路：BFS。每一輪先記下 queue 目前的長度 = 這一層的節點數，只處理這麼多個，就能把層分開。
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
int **levelOrder(struct TreeNode *root, int *returnSize, int **returnColumnSizes)
{
    enum { MAXN = 2000 }; /* 題目：最多 2000 個節點 */
    struct TreeNode **q = malloc(MAXN * sizeof *q);
    int **ans = malloc(MAXN * sizeof *ans);
    *returnColumnSizes = malloc(MAXN * sizeof **returnColumnSizes);
    int head = 0, tail = 0, levels = 0;
    if (root)
        q[tail++] = root;
    while (head < tail) {
        int width = tail - head; /* 這一層有幾個 */
        ans[levels] = malloc((size_t)width * sizeof **ans);
        (*returnColumnSizes)[levels] = width;
        for (int i = 0; i < width; i++) {
            struct TreeNode *t = q[head++];
            ans[levels][i] = t->val;
            if (t->left)
                q[tail++] = t->left;
            if (t->right)
                q[tail++] = t->right;
        }
        levels++;
    }
    free(q);
    *returnSize = levels;
    return ans;
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
    int m, *cols;
    int **r = levelOrder(t, &m, &cols);
    assert(m == 3);
    assert(cols[0] == 1 && r[0][0] == 3);
    assert(cols[1] == 2 && r[1][0] == 9 && r[1][1] == 20);
    assert(cols[2] == 2 && r[2][0] == 15 && r[2][1] == 7);
    for (int i = 0; i < m; i++)
        free(r[i]);
    free(r);
    free(cols);
    free_tree(t);

    r = levelOrder(NULL, &m, &cols);
    assert(m == 0);
    free(r);
    free(cols);
    puts("0102: passed");
    return 0;
}
