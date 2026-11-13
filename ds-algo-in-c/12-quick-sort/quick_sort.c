#include "quick_sort.h"

#include <assert.h>
#include <stdlib.h>

static void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

/* CLRS PARTITION：i 是「<= pivot 區」的最後一格。
 *   [lo .. i]    <= pivot
 *   [i+1 .. j-1] >  pivot
 *   [j .. hi-1]  還沒看
 *   hi           pivot */
int lomuto_partition(int *a, int lo, int hi)
{
    int pivot = a[hi], i = lo - 1;
    for (int j = lo; j < hi; j++)
        if (a[j] <= pivot)
            swap(&a[++i], &a[j]);
    swap(&a[i + 1], &a[hi]);
    return i + 1;
}

/* Hoare：兩個指標從兩端往中間走，遇到放錯邊的就交換。交換次數約是 Lomuto 的 1/3。 */
int hoare_partition(int *a, int lo, int hi)
{
    int pivot = a[lo], i = lo - 1, j = hi + 1;
    for (;;) {
        do
            i++;
        while (a[i] < pivot);
        do
            j--;
        while (a[j] > pivot);
        if (i >= j)
            return j;
        swap(&a[i], &a[j]);
    }
}

static void lomuto_rec(int *a, int lo, int hi)
{
    while (lo < hi) {
        int p = lomuto_partition(a, lo, hi);
        /* 先遞迴比較小的那一半，大的那一半用迴圈處理：堆疊深度最多 O(log n) */
        if (p - lo < hi - p) {
            lomuto_rec(a, lo, p - 1);
            lo = p + 1;
        } else {
            lomuto_rec(a, p + 1, hi);
            hi = p - 1;
        }
    }
}

void quick_sort_lomuto(int *a, int n)
{
    lomuto_rec(a, 0, n - 1);
}

static int rand_between(int lo, int hi) /* [lo, hi] */
{
    return lo + (int)((unsigned)rand() % (unsigned)(hi - lo + 1));
}

static void random_rec(int *a, int lo, int hi)
{
    while (lo < hi) {
        swap(&a[lo], &a[rand_between(lo, hi)]); /* 隨機 pivot 換到 a[lo] */
        int j = hoare_partition(a, lo, hi);
        if (j - lo < hi - j) {
            random_rec(a, lo, j);
            lo = j + 1;
        } else {
            random_rec(a, j + 1, hi);
            hi = j;
        }
    }
}

void quick_sort_random(int *a, int n)
{
    random_rec(a, 0, n - 1);
}

/* Dijkstra 的荷蘭國旗 (Dutch national flag) 三路切分：
 *   [lo .. lt-1] < pivot,  [lt .. i-1] == pivot,  [i .. gt] 還沒看,  [gt+1 .. hi] > pivot */
static void three_way_rec(int *a, int lo, int hi)
{
    while (lo < hi) {
        swap(&a[lo], &a[rand_between(lo, hi)]);
        int pivot = a[lo], lt = lo, i = lo, gt = hi;
        while (i <= gt) {
            if (a[i] < pivot)
                swap(&a[lt++], &a[i++]);
            else if (a[i] > pivot)
                swap(&a[i], &a[gt--]); /* 換過來的還沒看，i 不動 */
            else
                i++;
        }
        /* 等於 pivot 的 [lt, gt] 已經在最終位置，不用再遞迴 */
        if (lt - lo < hi - gt) {
            three_way_rec(a, lo, lt - 1);
            lo = gt + 1;
        } else {
            three_way_rec(a, gt + 1, hi);
            hi = lt - 1;
        }
    }
}

void quick_sort_3way(int *a, int n)
{
    three_way_rec(a, 0, n - 1);
}

int quickselect(int *a, int n, int k)
{
    assert(k >= 0 && k < n);
    int lo = 0, hi = n - 1;
    while (lo < hi) {
        swap(&a[hi], &a[rand_between(lo, hi)]);
        int p = lomuto_partition(a, lo, hi); /* pivot 已經在最終位置 p */
        if (p == k)
            return a[p];
        if (k < p)
            hi = p - 1; /* 只要往答案那一邊走，不像排序要兩邊都做 */
        else
            lo = p + 1;
    }
    return a[lo];
}
