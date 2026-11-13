/* LeetCode 215 · Kth Largest Element in an Array（Quickselect 版）
 * 思路：第 k 大 = 排序後 index n-k 的元素。用 partition 找 pivot 的最終位置 p，
 *       只往答案那一邊繼續找。平均 O(n)。
 *       13 主題寫過 heap 版（O(n log k)）。
 *   注意：這題的測資有「大量相同值」，兩路切分會退化成 O(n²)，所以用三路切分。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

int findKthLargest(int *nums, int numsSize, int k)
{
    int target = numsSize - k; /* 由小到大的 index */
    int lo = 0, hi = numsSize - 1;
    while (lo < hi) {
        swap(&nums[lo], &nums[lo + rand() % (hi - lo + 1)]);
        int pivot = nums[lo], lt = lo, i = lo, gt = hi;
        while (i <= gt) {
            if (nums[i] < pivot)
                swap(&nums[lt++], &nums[i++]);
            else if (nums[i] > pivot)
                swap(&nums[i], &nums[gt--]);
            else
                i++;
        }
        /* [lt, gt] 都等於 pivot，已經在最終位置 */
        if (target < lt)
            hi = lt - 1;
        else if (target > gt)
            lo = gt + 1;
        else
            return pivot;
    }
    return nums[lo];
}
/* ===== 提交範圍 結束 ===== */

static int cmp_desc(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a < b) - (a > b);
}

int main(void)
{
    int a[] = {3, 2, 1, 5, 6, 4};
    assert(findKthLargest(a, 6, 2) == 5);
    int b[] = {3, 2, 3, 1, 2, 4, 5, 5, 6};
    assert(findKthLargest(b, 9, 4) == 4);
    srand(215);
    for (int n = 1; n <= 80; n++)
        for (int k = 1; k <= n; k++) {
            int x[80], y[80];
            for (int i = 0; i < n; i++)
                x[i] = y[i] = rand() % 7;
            qsort(y, (size_t)n, sizeof *y, cmp_desc);
            assert(findKthLargest(x, n, k) == y[k - 1]);
        }
    static int same[100000]; /* 全部相同：三路切分一步就結束 */
    for (int i = 0; i < 100000; i++)
        same[i] = 1;
    assert(findKthLargest(same, 100000, 50000) == 1);
    puts("0215: passed");
    return 0;
}
