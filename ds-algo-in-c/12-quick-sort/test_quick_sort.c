#include "quick_sort.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static void test_partitions(void)
{
    srand(12);
    for (int t = 0; t < 2000; t++) {
        int n = 1 + rand() % 40, a[40], b[40];
        for (int i = 0; i < n; i++)
            a[i] = b[i] = rand() % 10;
        int pivot = a[n - 1];
        int p = lomuto_partition(a, 0, n - 1);
        assert(a[p] == pivot);
        for (int i = 0; i < n; i++)
            assert(i <= p ? a[i] <= pivot : a[i] > pivot);

        pivot = b[0];
        int j = hoare_partition(b, 0, n - 1);
        assert(j >= 0 && j < n);
        for (int i = 0; i < n; i++)
            assert(i <= j ? b[i] <= pivot : b[i] >= pivot);
    }
}

typedef void (*SortFn)(int *, int);

static void test_sorts(void)
{
    SortFn fns[] = {quick_sort_lomuto, quick_sort_random, quick_sort_3way};
    srand(1212);
    for (int t = 0; t < 3000; t++) {
        int n = rand() % 80, base[80], want[80];
        int range = t % 3 == 0 ? 2 : 1000;
        for (int i = 0; i < n; i++)
            base[i] = want[i] = rand() % range - range / 2;
        qsort(want, (size_t)n, sizeof *want, cmp_int);
        for (int f = 0; f < 3; f++) {
            int a[80];
            memcpy(a, base, sizeof a);
            fns[f](a, n);
            assert(n == 0 || memcmp(a, want, (size_t)n * sizeof *a) == 0);
        }
    }
}

/* 大量已排序、全部相同的資料：隨機 pivot 和三路切分都要很快，而且不能爆堆疊 */
static void test_adversarial(void)
{
    enum { N = 200000 };
    int *a = malloc(N * sizeof *a);
    for (int i = 0; i < N; i++)
        a[i] = i;
    quick_sort_random(a, N);
    for (int i = 0; i < N; i++)
        assert(a[i] == i);
    for (int i = 0; i < N; i++)
        a[i] = 7;
    quick_sort_3way(a, N);
    quick_sort_random(a, N);
    for (int i = 0; i < N; i++)
        assert(a[i] == 7);
    /* Lomuto 固定 pivot 在已排序輸入是 O(n²)，但「先遞迴小的一邊」讓堆疊只有 O(log n)，不會當掉 */
    for (int i = 0; i < 20000; i++)
        a[i] = i;
    quick_sort_lomuto(a, 20000);
    for (int i = 0; i < 20000; i++)
        assert(a[i] == i);
    free(a);
}

static void test_quickselect(void)
{
    srand(99);
    for (int t = 0; t < 1000; t++) {
        int n = 1 + rand() % 50, a[50], sorted[50];
        for (int i = 0; i < n; i++)
            a[i] = sorted[i] = rand() % 20;
        qsort(sorted, (size_t)n, sizeof *sorted, cmp_int);
        int k = rand() % n;
        assert(quickselect(a, n, k) == sorted[k]);
    }
}

int main(void)
{
    test_partitions();
    test_sorts();
    test_adversarial();
    test_quickselect();
    puts("quick_sort: all tests passed");
    return 0;
}
