#include "simple_sort.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static long inversions(const int *a, int n)
{
    long c = 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            c += a[i] > a[j];
    return c;
}

typedef void (*SortFn)(int *, int);

static void insertion_wrapper(int *a, int n) { insertion_sort(a, n); }

static void test_all_sorts(void)
{
    SortFn fns[] = {insertion_wrapper, binary_insertion_sort, bubble_sort, selection_sort, shell_sort};
    srand(10);
    for (int t = 0; t < 2000; t++) {
        int n = rand() % 60, base[60], want[60];
        int range = t % 3 == 0 ? 3 : 1000; /* 有時候很多重複值 */
        for (int i = 0; i < n; i++)
            base[i] = want[i] = rand() % range - range / 2;
        qsort(want, (size_t)n, sizeof *want, cmp_int);
        for (int f = 0; f < 5; f++) {
            int a[60];
            memcpy(a, base, sizeof a);
            fns[f](a, n);
            assert(is_sorted(a, n));
            assert(n == 0 || memcmp(a, want, (size_t)n * sizeof *a) == 0);
        }
    }
}

static void test_insertion_moves(void)
{
    int sorted[] = {1, 2, 3, 4, 5};
    assert(insertion_sort(sorted, 5) == 0); /* 已排序：0 次搬移，最好情況 O(n) */
    int rev[] = {5, 4, 3, 2, 1};
    assert(insertion_sort(rev, 5) == 10); /* 反序：n(n-1)/2，最壞情況 */

    srand(100);
    for (int t = 0; t < 500; t++) {
        int n = rand() % 40, a[40];
        for (int i = 0; i < n; i++)
            a[i] = rand() % 20;
        long inv = inversions(a, n);
        assert(insertion_sort(a, n) == inv);
    }
}

int main(void)
{
    test_all_sorts();
    test_insertion_moves();
    puts("simple_sort: all tests passed");
    return 0;
}
