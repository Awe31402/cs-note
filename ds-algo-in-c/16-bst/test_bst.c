#include "bst.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void test_example(void)
{
    /* CLRS Figure 12.2 的樹 */
    int keys[] = {15, 6, 18, 3, 7, 17, 20, 2, 4, 13, 9};
    BST t;
    bst_init(&t);
    for (int i = 0; i < 11; i++)
        assert(bst_insert(&t, keys[i]) == 1);
    assert(bst_insert(&t, 7) == 0);
    assert(bst_check(&t) && t.size == 11);

    assert(bst_min(t.root)->key == 2 && bst_max(t.root)->key == 20);
    assert(bst_successor(bst_search(&t, 15))->key == 17); /* 有右子樹 */
    assert(bst_successor(bst_search(&t, 13))->key == 15); /* 沒有右子樹：往上找 */
    assert(bst_successor(bst_search(&t, 20)) == NULL);
    assert(bst_predecessor(bst_search(&t, 9))->key == 7);
    assert(bst_floor(&t, 14)->key == 13 && bst_ceil(&t, 14)->key == 15);
    assert(bst_floor(&t, 1) == NULL && bst_ceil(&t, 21) == NULL);

    assert(bst_delete(&t, 2));  /* 葉子 */
    assert(bst_delete(&t, 13)); /* 只有左小孩 */
    assert(bst_delete(&t, 6));  /* 兩個小孩，後繼 7 剛好是它的右小孩（y->parent == z） */
    assert(bst_delete(&t, 15)); /* 根，兩個小孩，後繼 17 不是直接右小孩 */
    assert(!bst_delete(&t, 99));
    assert(bst_check(&t) && t.size == 7);
    int out[16], want[] = {3, 4, 7, 9, 17, 18, 20};
    assert(bst_inorder(&t, out) == 7);
    for (int i = 0; i < 7; i++)
        assert(out[i] == want[i]);
    bst_free(&t);
}

static void test_random(void)
{
    enum { R = 300 };
    static int present[R];
    BST t;
    bst_init(&t);
    srand(16);
    for (int step = 0; step < 50000; step++) {
        int k = rand() % R;
        if (rand() % 2) {
            assert(bst_insert(&t, k) == !present[k]);
            present[k] = 1;
        } else {
            assert(bst_delete(&t, k) == present[k]);
            present[k] = 0;
        }
        if (step % 50 == 0) {
            assert(bst_check(&t));
            int out[R], n = bst_inorder(&t, out), j = 0;
            for (int v = 0; v < R; v++)
                if (present[v])
                    assert(out[j++] == v);
            assert(j == n && n == t.size);
            int q = rand() % (R + 20) - 10;
            BNode *f = bst_floor(&t, q), *c = bst_ceil(&t, q);
            int wf = -1, wc = -1;
            for (int v = 0; v < R; v++)
                if (present[v]) {
                    if (v <= q)
                        wf = v;
                    if (v >= q && wc < 0)
                        wc = v;
                }
            assert(f ? f->key == wf : wf < 0);
            assert(c ? c->key == wc : wc < 0);
            /* 用後繼走完整棵樹，順序要跟中序一樣 */
            j = 0;
            for (BNode *x = bst_min(t.root); x; x = bst_successor(x))
                assert(x->key == out[j++]);
            assert(j == n);
        }
    }
    bst_free(&t);
}

static void test_degenerate(void)
{
    BST t;
    bst_init(&t);
    for (int i = 0; i < 1000; i++)
        bst_insert(&t, i); /* 已排序的輸入：變成一條直線 */
    assert(bst_height(&t) == 1000);
    bst_free(&t);
}

int main(void)
{
    test_example();
    test_random();
    test_degenerate();
    puts("bst: all tests passed");
    return 0;
}
