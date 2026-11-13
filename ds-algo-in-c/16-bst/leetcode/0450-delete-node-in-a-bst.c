/* LeetCode 450 · Delete Node in a BST
 * 思路（遞迴、沒有 parent 指標）：先找到要刪的節點，然後：
 *   - 沒有左小孩 → 回傳右小孩頂替
 *   - 沒有右小孩 → 回傳左小孩頂替
 *   - 兩個都有   → 把右子樹最小的（後繼）值搬上來，再到右子樹刪掉那個後繼
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
struct TreeNode *deleteNode(struct TreeNode *root, int key)
{
    if (!root)
        return NULL;
    if (key < root->val) {
        root->left = deleteNode(root->left, key);
    } else if (key > root->val) {
        root->right = deleteNode(root->right, key);
    } else if (!root->left || !root->right) {
        struct TreeNode *child = root->left ? root->left : root->right;
        free(root);
        return child;
    } else {
        struct TreeNode *s = root->right;
        while (s->left)
            s = s->left;
        root->val = s->val; /* 後繼的值搬上來 */
        root->right = deleteNode(root->right, s->val);
    }
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

static int inorder(struct TreeNode *r, int *out, int k)
{
    if (!r)
        return k;
    k = inorder(r->left, out, k);
    out[k++] = r->val;
    return inorder(r->right, out, k);
}

int main(void)
{
    srand(450);
    for (int t = 0; t < 300; t++) {
        int present[60] = {0};
        struct TreeNode *r = NULL;
        for (int i = 0; i < 40; i++) {
            int v = rand() % 60;
            r = insert(r, v);
            present[v] = 1;
        }
        for (int i = 0; i < 30; i++) {
            int v = rand() % 60;
            r = deleteNode(r, v); /* 不存在的也要能處理 */
            present[v] = 0;
            int out[60], n = inorder(r, out, 0), j = 0;
            for (int x = 0; x < 60; x++)
                if (present[x])
                    assert(j < n && out[j++] == x);
            assert(j == n);
        }
        free_tree(r);
    }
    puts("0450: passed");
    return 0;
}
