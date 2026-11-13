#include "rbtree.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void test_small(void)
{
    RBTree t;
    rb_init(&t);
    assert(rb_check(&t) == 1); /* 空樹：只有 NIL */
    /* CLRS Figure 13.4 的插入例子 */
    int keys[] = {11, 2, 14, 1, 7, 15, 5, 8, 4};
    for (int i = 0; i < 9; i++) {
        assert(rb_insert(&t, keys[i]) == 1);
        assert(rb_check(&t) > 0);
    }
    assert(rb_insert(&t, 7) == 0);
    assert(t.root->key == 7); /* 插入 4 之後，7 會變成根（跟書上的圖一樣） */
    assert(rb_search(&t, 5) && !rb_search(&t, 6));
    assert(rb_lower_bound(&t, 6)->key == 7 && rb_lower_bound(&t, 16) == NULL);
    for (int i = 0; i < 9; i++) {
        assert(rb_delete(&t, keys[i]) == 1);
        assert(rb_check(&t) > 0);
    }
    assert(t.size == 0 && t.root == &t.nil);
    rb_free(&t);
}

static void test_sorted(void)
{
    RBTree t;
    rb_init(&t);
    for (int i = 0; i < 100000; i++)
        rb_insert(&t, i);
    assert(rb_check(&t) > 0);
    assert(rb_height(&t) <= 2 * log2(100001.0)); /* 普通 BST 會是 100000 */
    for (int i = 0; i < 100000; i += 2)
        rb_delete(&t, i);
    assert(rb_check(&t) > 0 && t.size == 50000);
    rb_free(&t);
}

static void test_random(void)
{
    enum { R = 3000 };
    static int present[R];
    RBTree t;
    rb_init(&t);
    srand(18);
    for (int step = 0; step < 200000; step++) {
        int k = rand() % R;
        if (rand() % 2) {
            assert(rb_insert(&t, k) == !present[k]);
            present[k] = 1;
        } else {
            assert(rb_delete(&t, k) == present[k]);
            present[k] = 0;
        }
        if (step % 200 == 0) {
            assert(rb_check(&t) > 0);
            assert(rb_height(&t) <= 2 * log2(t.size + 1.0) + 1e-9);
            int out[R], n = rb_inorder(&t, out), j = 0;
            for (int v = 0; v < R; v++)
                if (present[v])
                    assert(out[j++] == v);
            assert(j == n);
            int q = rand() % (R + 10) - 5, want = -1;
            for (int v = q < 0 ? 0 : q; v < R; v++)
                if (present[v]) {
                    want = v;
                    break;
                }
            RBNode *lb = rb_lower_bound(&t, q);
            assert(lb ? lb->key == want : want < 0);
        }
    }
    rb_free(&t);
}

int main(void)
{
    test_small();
    test_sorted();
    test_random();
    puts("rbtree: all tests passed");
    return 0;
}
