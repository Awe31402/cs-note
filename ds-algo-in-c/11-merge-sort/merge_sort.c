#include "merge_sort.h"

#include <stdlib.h>
#include <string.h>

/* 合併 a[lo..mid) 和 a[mid..hi)，兩段都已排序。回傳這次合併發現的逆序對數。 */
static long long merge(int *a, int *tmp, int lo, int mid, int hi)
{
    int i = lo, j = mid, k = lo;
    long long inv = 0;
    while (i < mid && j < hi) {
        if (a[i] <= a[j]) { /* <= 讓左邊先：穩定 */
            tmp[k++] = a[i++];
        } else {
            tmp[k++] = a[j++];
            inv += mid - i; /* a[j] 比左半剩下的 mid - i 個都小 */
        }
    }
    while (i < mid)
        tmp[k++] = a[i++];
    while (j < hi)
        tmp[k++] = a[j++];
    memcpy(a + lo, tmp + lo, (size_t)(hi - lo) * sizeof *a);
    return inv;
}

static long long sort_rec(int *a, int *tmp, int lo, int hi)
{
    if (hi - lo < 2) /* 0 或 1 個：已排好 */
        return 0;
    int mid = lo + (hi - lo) / 2;
    long long inv = sort_rec(a, tmp, lo, mid) + sort_rec(a, tmp, mid, hi);
    if (a[mid - 1] <= a[mid]) /* 優化：兩段已經接得起來，不用合併 */
        return inv;
    return inv + merge(a, tmp, lo, mid, hi);
}

static int *alloc_tmp(int n)
{
    int *tmp = malloc((size_t)(n > 0 ? n : 1) * sizeof *tmp); /* 只配置一次，不在遞迴裡 malloc */
    if (!tmp)
        abort();
    return tmp;
}

void merge_sort(int *a, int n)
{
    int *tmp = alloc_tmp(n);
    sort_rec(a, tmp, 0, n);
    free(tmp);
}

long long count_inversions(int *a, int n)
{
    int *tmp = alloc_tmp(n);
    long long inv = sort_rec(a, tmp, 0, n);
    free(tmp);
    return inv;
}

void merge_sort_bottom_up(int *a, int n)
{
    int *tmp = alloc_tmp(n);
    for (int width = 1; width < n; width *= 2)
        for (int lo = 0; lo < n - width; lo += 2 * width) {
            int mid = lo + width;
            int hi = mid + width < n ? mid + width : n;
            merge(a, tmp, lo, mid, hi);
        }
    free(tmp);
}
