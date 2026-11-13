#include "array.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void expect(const Vec *v, const int *want, int n)
{
    assert(v->size == n);
    for (int i = 0; i < n; i++)
        assert(v->data[i] == want[i]);
}

static void test_vec(void)
{
    Vec v;
    vec_init(&v);
    for (int i = 1; i <= 5; i++)
        vec_push(&v, i * 10); /* 10 20 30 40 50 */
    vec_insert(&v, 0, 5);     /* 頭 */
    vec_insert(&v, 3, 25);    /* 中間 */
    vec_insert(&v, v.size, 99); /* 尾 */
    int w1[] = {5, 10, 20, 25, 30, 40, 50, 99};
    expect(&v, w1, 8);

    assert(vec_erase(&v, 0) == 5);
    assert(vec_erase(&v, 2) == 25);
    assert(vec_erase(&v, v.size - 1) == 99);
    int w2[] = {10, 20, 30, 40, 50};
    expect(&v, w2, 5);

    assert(vec_find(&v, 30) == 2);
    assert(vec_find(&v, 31) == -1);
    assert(vec_pop(&v) == 50);
    assert(v.size == 4);

    for (int i = 0; i < 10000; i++) /* 擴容很多次 */
        vec_push(&v, i);
    assert(v.size == 10004 && v.data[10003] == 9999);
    vec_free(&v);
}

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static void test_merge(void)
{
    int a[] = {1, 3, 5, 7}, b[] = {2, 3, 6};
    int out[7], want[] = {1, 2, 3, 3, 5, 6, 7};
    assert(merge_sorted(a, 4, b, 3, out) == 7);
    assert(memcmp(out, want, sizeof want) == 0);
    assert(merge_sorted(a, 0, b, 3, out) == 3 && out[0] == 2);

    srand(3);
    for (int t = 0; t < 200; t++) {
        int n = rand() % 20, m = rand() % 20, x[20], y[20], got[40], all[40];
        for (int i = 0; i < n; i++)
            all[i] = x[i] = rand() % 50;
        for (int i = 0; i < m; i++)
            all[n + i] = y[i] = rand() % 50;
        qsort(x, (size_t)n, sizeof *x, cmp_int);
        qsort(y, (size_t)m, sizeof *y, cmp_int);
        qsort(all, (size_t)(n + m), sizeof *all, cmp_int);
        merge_sorted(x, n, y, m, got);
        assert(n + m == 0 || memcmp(got, all, (size_t)(n + m) * sizeof *got) == 0);
    }
}

static void test_matrix(void)
{
    /* 2×3 */
    int a[] = {1, 2, 3,
               4, 5, 6};
    assert(row_major_index(1, 2, 3) == 5 && a[row_major_index(1, 2, 3)] == 6);

    int t[6], want_t[] = {1, 4,
                          2, 5,
                          3, 6};
    mat_transpose(a, 2, 3, t);
    assert(memcmp(t, want_t, sizeof t) == 0);

    int p[4], want_p[] = {14, 32,
                          32, 77}; /* A · Aᵀ */
    mat_mul(a, t, 2, 3, 2, p);
    assert(memcmp(p, want_p, sizeof p) == 0);
}

static void test_sparse(void)
{
    int a[] = {0, 0, 3, 0,
               0, 0, 0, 0,
               7, 0, 0, -1};
    Triplet tr[12];
    int k = sparse_from_dense(a, 3, 4, tr);
    assert(k == 3);
    assert(tr[0].r == 0 && tr[0].c == 2 && tr[0].v == 3);
    assert(tr[2].r == 2 && tr[2].c == 3 && tr[2].v == -1);
    int back[12];
    sparse_to_dense(tr, k, 3, 4, back);
    assert(memcmp(a, back, sizeof a) == 0);
}

int main(void)
{
    test_vec();
    test_merge();
    test_matrix();
    test_sparse();
    puts("array: all tests passed");
    return 0;
}
