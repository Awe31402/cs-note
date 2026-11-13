#include "linear_sort.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static int cmp_double(const void *x, const void *y)
{
    double a = *(const double *)x, b = *(const double *)y;
    return (a > b) - (a < b);
}

static void test_counting_and_radix(void)
{
    srand(14);
    for (int t = 0; t < 2000; t++) {
        int n = rand() % 100, a[100], b[100], out[100], want[100];
        for (int i = 0; i < n; i++) {
            int v = t % 2 ? rand() % 50 - 25 : rand() - RAND_MAX / 2;
            a[i] = b[i] = want[i] = v;
        }
        qsort(want, (size_t)n, sizeof *want, cmp_int);
        if (t % 2) /* 範圍小才用計數排序，範圍大會開一個巨大的計數陣列 */
            counting_sort(a, n, out);
        else
            memcpy(out, want, sizeof out);
        radix_sort(b, n);
        assert(n == 0 || memcmp(out, want, (size_t)n * sizeof *out) == 0);
        assert(n == 0 || memcmp(b, want, (size_t)n * sizeof *b) == 0);
    }
    int ext[] = {INT_MAX, 0, INT_MIN, -1, 1, INT_MIN, INT_MAX};
    int want[] = {INT_MIN, INT_MIN, -1, 0, 1, INT_MAX, INT_MAX};
    radix_sort(ext, 7);
    assert(memcmp(ext, want, sizeof want) == 0);
}

static void test_stability(void)
{
    Rec in[200], out[200];
    srand(141);
    for (int i = 0; i < 200; i++)
        in[i] = (Rec){rand() % 5, i}; /* id 就是原本的順序 */
    counting_sort_rec(in, 200, 4, out);
    for (int i = 1; i < 200; i++) {
        assert(out[i - 1].key <= out[i].key);
        if (out[i - 1].key == out[i].key)
            assert(out[i - 1].id < out[i].id); /* 同 key 保持原順序 */
    }
}

static void test_bucket(void)
{
    srand(1414);
    for (int t = 0; t < 500; t++) {
        int n = rand() % 200;
        double a[200], want[200];
        for (int i = 0; i < n; i++)
            a[i] = want[i] = (double)rand() / ((double)RAND_MAX + 1.0); /* [0, 1) */
        qsort(want, (size_t)n, sizeof *want, cmp_double);
        bucket_sort(a, n);
        assert(n == 0 || memcmp(a, want, (size_t)n * sizeof *a) == 0);
    }
}

int main(void)
{
    test_counting_and_radix();
    test_stability();
    test_bucket();
    puts("linear_sort: all tests passed");
    return 0;
}
