#include "btree.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void test_small(void)
{
    BTree b;
    bt_init(&b, 2); /* 2-3-4 樹 */
    for (int k = 1; k <= 10; k++)
        assert(bt_insert(&b, k));
    assert(!bt_insert(&b, 5) && bt_check(&b));
    int out[16];
    assert(bt_inorder(&b, out) == 10);
    for (int i = 0; i < 10; i++)
        assert(out[i] == i + 1);
    int v;
    assert(bt_lower_bound(&b, 0, &v) && v == 1);
    assert(bt_lower_bound(&b, 10, &v) && v == 10);
    assert(!bt_lower_bound(&b, 11, &v));
    for (int k = 10; k >= 1; k--) { /* 刪到空：一路觸發借、合併、根縮小 */
        assert(bt_delete(&b, k) && bt_check(&b));
        assert(!bt_search(&b, k));
    }
    assert(b.size == 0 && b.root->leaf && b.root->n == 0);
    assert(!bt_delete(&b, 3));
    bt_free(&b);
}

static void test_random(int t)
{
    enum { R = 3000 };
    static int present[R];
    for (int i = 0; i < R; i++)
        present[i] = 0;
    BTree b;
    bt_init(&b, t);
    srand(31 + t);
    for (int step = 0; step < 100000; step++) {
        int k = rand() % R;
        if (rand() % 2) {
            assert(bt_insert(&b, k) == !present[k]);
            present[k] = 1;
        } else {
            assert(bt_delete(&b, k) == present[k]);
            present[k] = 0;
        }
        int q = rand() % R;
        assert(bt_search(&b, q) == present[q]);
        if (step % 250 == 0) {
            assert(bt_check(&b));
            int out[R], n = bt_inorder(&b, out), j = 0;
            for (int v = 0; v < R; v++)
                if (present[v])
                    assert(out[j++] == v);
            assert(j == n);
            /* CLRS Theorem 18.1：高度 h <= log_t((n + 1) / 2) + 1（本專案葉子算高度 1） */
            if (n > 0)
                assert(bt_height(&b) <= log((n + 1) / 2.0) / log((double)t) + 1 + 1e-9);
            int lb, want = -1;
            for (int v = q; v < R; v++)
                if (present[v]) {
                    want = v;
                    break;
                }
            assert(bt_lower_bound(&b, q, &lb) ? lb == want : want < 0);
        }
    }
    bt_free(&b);
}

int main(void)
{
    test_small();
    for (int t = 2; t <= 6; t++)
        test_random(t);
    puts("btree: all tests passed");
    return 0;
}
