#include "merge_sort.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static long long brute_inv(const int *a, int n)
{
    long long c = 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            c += a[i] > a[j];
    return c;
}

static void test_random(void)
{
    srand(11);
    for (int t = 0; t < 2000; t++) {
        int n = rand() % 100, a[100], b[100], c[100], want[100];
        int range = t % 2 ? 5 : 100000;
        for (int i = 0; i < n; i++)
            a[i] = b[i] = c[i] = want[i] = rand() % range - range / 2;
        long long inv = brute_inv(a, n);
        qsort(want, (size_t)n, sizeof *want, cmp_int);
        merge_sort(a, n);
        merge_sort_bottom_up(b, n);
        assert(count_inversions(c, n) == inv);
        assert(n == 0 || memcmp(a, want, (size_t)n * sizeof *a) == 0);
        assert(n == 0 || memcmp(b, want, (size_t)n * sizeof *b) == 0);
        assert(n == 0 || memcmp(c, want, (size_t)n * sizeof *c) == 0);
    }
}

static void test_large(void)
{
    enum { N = 200000 };
    int *a = malloc(N * sizeof *a), *b = malloc(N * sizeof *b);
    for (int i = 0; i < N; i++)
        a[i] = b[i] = N - i; /* 反序：逆序對 = N(N-1)/2，超過 int */
    assert(count_inversions(a, N) == (long long)N * (N - 1) / 2);
    merge_sort_bottom_up(b, N);
    for (int i = 0; i < N; i++)
        assert(a[i] == i + 1 && b[i] == i + 1);
    free(a);
    free(b);
}

int main(void)
{
    test_random();
    test_large();
    puts("merge_sort: all tests passed");
    return 0;
}
