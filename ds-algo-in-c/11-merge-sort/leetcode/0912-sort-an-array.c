/* LeetCode 912 · Sort an Array（不能用內建排序，要 O(n log n)）
 * 思路：merge sort。最壞也是 O(n log n)，不怕特別設計的測資（quicksort 固定 pivot 會被卡成 O(n²)）。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
static void sort_rec(int *a, int *tmp, int lo, int hi)
{
    if (hi - lo < 2)
        return;
    int mid = lo + (hi - lo) / 2;
    sort_rec(a, tmp, lo, mid);
    sort_rec(a, tmp, mid, hi);
    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi)
        tmp[k++] = a[i] <= a[j] ? a[i++] : a[j++];
    while (i < mid)
        tmp[k++] = a[i++];
    while (j < hi)
        tmp[k++] = a[j++];
    memcpy(a + lo, tmp + lo, (size_t)(hi - lo) * sizeof *a);
}

int *sortArray(int *nums, int numsSize, int *returnSize)
{
    int *tmp = malloc((size_t)numsSize * sizeof *tmp);
    sort_rec(nums, tmp, 0, numsSize);
    free(tmp);
    *returnSize = numsSize;
    return nums; /* 原地排好，直接回傳 */
}
/* ===== 提交範圍 結束 ===== */

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

int main(void)
{
    int a[] = {5, 2, 3, 1}, wa[] = {1, 2, 3, 5}, m;
    assert(memcmp(sortArray(a, 4, &m), wa, sizeof wa) == 0 && m == 4);
    int b[] = {5, 1, 1, 2, 0, 0}, wb[] = {0, 0, 1, 1, 2, 5};
    assert(memcmp(sortArray(b, 6, &m), wb, sizeof wb) == 0);
    srand(912);
    for (int t = 0; t < 300; t++) {
        int n = 1 + rand() % 200, x[200], y[200];
        for (int i = 0; i < n; i++)
            x[i] = y[i] = rand() % 100001 - 50000;
        qsort(y, (size_t)n, sizeof *y, cmp_int);
        sortArray(x, n, &m);
        assert(memcmp(x, y, (size_t)n * sizeof *x) == 0);
    }
    puts("0912: passed");
    return 0;
}
