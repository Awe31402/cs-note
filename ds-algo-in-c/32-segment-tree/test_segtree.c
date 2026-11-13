#include "segtree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void test_random(void)
{
    srand(32);
    for (int t = 0; t < 200; t++) {
        int n = 1 + rand() % 60, a[60];
        long long model[60];
        for (int i = 0; i < n; i++)
            model[i] = a[i] = rand() % 21 - 10;
        SegTree st;
        Fenwick fw;
        st_init(&st, a, n);
        for (int step = 0; step < 500; step++) {
            int l = rand() % n, r = rand() % n;
            if (l > r) {
                int x = l;
                l = r;
                r = x;
            }
            int op = rand() % 3;
            if (op == 0) {
                long long v = rand() % 1001 - 500;
                st_add(&st, l, r, v);
                for (int i = l; i <= r; i++)
                    model[i] += v;
            } else if (op == 1) {
                long long v = rand() % 1001 - 500;
                st_set(&st, l, v);
                model[l] = v;
            }
            long long want = 0;
            for (int i = l; i <= r; i++)
                want += model[i];
            assert(st_sum(&st, l, r) == want);
        }
        /* 樹狀陣列只支援單點加值：用最後的內容建一棵，跟線段樹一起檢查所有區間 */
        fw_init(&fw, n);
        for (int i = 0; i < n; i++)
            fw_add(&fw, i, model[i]);
        for (int l = 0; l < n; l++)
            for (int r = l; r < n; r++) {
                long long want = 0;
                for (int i = l; i <= r; i++)
                    want += model[i];
                assert(fw_prefix(&fw, r) - fw_prefix(&fw, l - 1) == want);
                assert(st_sum(&st, l, r) == want);
            }
        st_free(&st);
        fw_free(&fw);
    }
}

static void test_large(void)
{
    enum { N = 100000 };
    int *a = calloc(N, sizeof *a);
    SegTree st;
    st_init(&st, a, N);
    for (int k = 0; k < 100000; k++) /* 每次都加整個陣列：懶標記讓它只要 O(1) 個節點 */
        st_add(&st, 0, N - 1, 1000000);
    assert(st_sum(&st, 0, N - 1) == 100000LL * 1000000 * N); /* 10^16，要 long long */
    assert(st_sum(&st, 123, 123) == 100000LL * 1000000);
    st_free(&st);
    free(a);
}

int main(void)
{
    test_random();
    test_large();
    puts("segtree: all tests passed");
    return 0;
}
