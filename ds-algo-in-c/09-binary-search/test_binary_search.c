#include "binary_search.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static void test_examples(void)
{
    int a[] = {1, 3, 3, 3, 5, 8, 13};
    int n = 7;
    assert(a[bs_find(a, n, 3)] == 3);
    assert(bs_find(a, n, 4) == -1);
    assert(bs_find(a, n, 0) == -1 && bs_find(a, n, 99) == -1);
    assert(lower_bound(a, n, 3) == 1 && upper_bound(a, n, 3) == 4); /* 3 出現 upper - lower = 3 次 */
    assert(lower_bound(a, n, 4) == 4 && upper_bound(a, n, 4) == 4); /* 不存在：兩個相等，就是插入點 */
    assert(lower_bound(a, n, 0) == 0 && lower_bound(a, n, 99) == 7);
    assert(bs_find(a, 0, 1) == -1 && lower_bound(a, 0, 1) == 0);
}

static void test_random(void)
{
    srand(9);
    for (int t = 0; t < 3000; t++) {
        int n = rand() % 40, a[40];
        for (int i = 0; i < n; i++)
            a[i] = rand() % 30;
        qsort(a, (size_t)n, sizeof *a, cmp_int);
        for (int x = -2; x < 33; x++) {
            int lin = linear_search(a, n, x);
            int found[] = {bs_find(a, n, x), bs_find_rec(a, 0, n, x),
                           interpolation_search(a, n, x), jump_search(a, n, x)};
            for (int k = 0; k < 4; k++) {
                assert((found[k] == -1) == (lin == -1));
                assert(found[k] == -1 || a[found[k]] == x);
            }
            int lb = lower_bound(a, n, x), ub = upper_bound(a, n, x);
            int want_lb = 0, want_ub = 0;
            while (want_lb < n && a[want_lb] < x)
                want_lb++;
            while (want_ub < n && a[want_ub] <= x)
                want_ub++;
            assert(lb == want_lb && ub == want_ub);
        }
    }
}

static void test_isqrt(void)
{
    for (int x = 0; x <= 100000; x++) {
        long long r = isqrt_bs(x);
        assert(r * r <= x && (r + 1) * (r + 1) > x);
    }
    assert(isqrt_bs(INT_MAX) == 46340);
    assert(isqrt_bs(2147395600) == 46340); /* 46340² */
}

static void test_interpolation_extremes(void)
{
    int a[] = {INT_MIN, -5, 0, 7, INT_MAX}; /* 相減會溢位的值 */
    for (int i = 0; i < 5; i++)
        assert(interpolation_search(a, 5, a[i]) == i);
    assert(interpolation_search(a, 5, 1) == -1);
    int same[] = {4, 4, 4};
    assert(interpolation_search(same, 3, 4) >= 0 && interpolation_search(same, 3, 5) == -1);
}

int main(void)
{
    test_examples();
    test_random();
    test_isqrt();
    test_interpolation_extremes();
    puts("binary_search: all tests passed");
    return 0;
}
