#include "binary_tree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N_ (-1) /* null 標記 */

static void expect(int got_n, const int *got, const int *want, int n)
{
    assert(got_n == n);
    assert(n == 0 || memcmp(got, want, (size_t)n * sizeof *got) == 0);
}

/*        1
 *      /   \
 *     2     3
 *    / \     \
 *   4   5     6
 *      /
 *     7          */
static void test_example(void)
{
    int lv[] = {1, 2, 3, 4, 5, N_, 6, N_, N_, 7};
    TNode *t = tree_from_level(lv, 10, N_);
    int out[16];
    int pre[] = {1, 2, 4, 5, 7, 3, 6}, in[] = {4, 2, 7, 5, 1, 3, 6};
    int post[] = {4, 7, 5, 2, 6, 3, 1}, lvl[] = {1, 2, 3, 4, 5, 6, 7};
    expect(preorder(t, out), out, pre, 7);
    expect(inorder(t, out), out, in, 7);
    expect(postorder(t, out), out, post, 7);
    expect(preorder_iter(t, out), out, pre, 7);
    expect(inorder_iter(t, out), out, in, 7);
    expect(postorder_iter(t, out), out, post, 7);
    expect(level_order(t, out), out, lvl, 7);
    assert(tree_size(t) == 7 && tree_height(t) == 4);
    tree_free(t);

    assert(tree_from_level(NULL, 0, N_) == NULL);
    assert(tree_size(NULL) == 0 && tree_height(NULL) == 0);
    assert(inorder_iter(NULL, out) == 0 && postorder_iter(NULL, out) == 0);
}

/* 隨機插入成一棵樹（不是 BST，只是隨機形狀），值 = 建立順序，保證不重複 */
static TNode *random_tree(int n)
{
    if (n == 0)
        return NULL;
    TNode *root = tn_new(0);
    for (int v = 1; v < n; v++) {
        TNode *p = root;
        for (;;) {
            TNode **child = rand() % 2 ? &p->left : &p->right;
            if (!*child) {
                *child = tn_new(v);
                break;
            }
            p = *child;
        }
    }
    return root;
}

static void test_random(void)
{
    srand(15);
    for (int t = 0; t < 500; t++) {
        int n = rand() % 60;
        TNode *root = random_tree(n);
        int a[64], b[64], pre[64], in[64];
        expect(preorder_iter(root, b), b, a, preorder(root, a));
        expect(inorder_iter(root, b), b, a, inorder(root, a));
        expect(postorder_iter(root, b), b, a, postorder(root, a));
        assert(level_order(root, a) == n);

        /* 前序 + 中序 → 重建 → 走訪結果要一樣 */
        preorder(root, pre);
        inorder(root, in);
        TNode *copy = build_pre_in(pre, in, n);
        expect(preorder(copy, a), a, pre, n);
        expect(inorder(copy, a), a, in, n);
        assert(tree_height(copy) == tree_height(root));
        tree_free(copy);
        tree_free(root);
    }
}

int main(void)
{
    test_example();
    test_random();
    puts("binary_tree: all tests passed");
    return 0;
}
