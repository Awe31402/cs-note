#include "avl.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void test_four_cases(void)
{
    int cases[4][3] = {{3, 2, 1},  /* LL → 一次右旋 */
                       {1, 2, 3},  /* RR → 一次左旋 */
                       {3, 1, 2},  /* LR → 兩次 */
                       {1, 3, 2}}; /* RL → 兩次 */
    long want_rot[4] = {1, 1, 2, 2};
    for (int c = 0; c < 4; c++) {
        AVL t;
        avl_init(&t);
        for (int i = 0; i < 3; i++)
            avl_insert(&t, cases[c][i]);
        assert(t.root->key == 2 && t.root->left->key == 1 && t.root->right->key == 3);
        assert(t.rotations == want_rot[c] && avl_check(&t));
        avl_free(&t);
    }
}

static void test_sorted_input(void)
{
    AVL t;
    avl_init(&t);
    for (int i = 1; i <= 1023; i++) /* 普通 BST 會變成高度 1023 的直線 */
        avl_insert(&t, i);
    assert(avl_check(&t));
    assert(avl_height(&t) == 10); /* 2^10 - 1 個節點，剛好是完美平衡 */
    avl_free(&t);
}

static void test_random(void)
{
    enum { R = 2000 };
    static int present[R];
    AVL t;
    avl_init(&t);
    srand(17);
    for (int step = 0; step < 100000; step++) {
        int k = rand() % R;
        if (rand() % 3) {
            assert(avl_insert(&t, k) == !present[k]);
            present[k] = 1;
        } else {
            assert(avl_delete(&t, k) == present[k]);
            present[k] = 0;
        }
        int q = rand() % R;
        assert(avl_contains(&t, q) == present[q]);
        if (step % 100 == 0) {
            assert(avl_check(&t));
            /* AVL 的高度上界：h < 1.4405 log2(n + 2) - 0.3277 */
            assert(avl_height(&t) <= 1.4405 * log2(t.size + 2.0));
            int out[R], n = avl_inorder(&t, out), j = 0;
            for (int v = 0; v < R; v++)
                if (present[v])
                    assert(out[j++] == v);
            assert(j == n);
        }
    }
    avl_free(&t);
}

int main(void)
{
    test_four_cases();
    test_sorted_input();
    test_random();
    puts("avl: all tests passed");
    return 0;
}
