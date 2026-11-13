/* LeetCode 215 · Kth Largest Element in an Array
 * 思路：大小為 k 的 min-heap。堆頂 = 目前看過的數字中第 k 大。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
static void sift_down_min(int *a, int n, int i)
{
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < n && a[l] < a[m])
            m = l;
        if (r < n && a[r] < a[m])
            m = r;
        if (m == i)
            return;
        int t = a[i];
        a[i] = a[m];
        a[m] = t;
        i = m;
    }
}

int findKthLargest(int *nums, int numsSize, int k)
{
    int *h = malloc((size_t)k * sizeof *h);
    memcpy(h, nums, (size_t)k * sizeof *h); /* 前 k 個先建成 min-heap */
    for (int i = k / 2 - 1; i >= 0; i--)
        sift_down_min(h, k, i);
    for (int i = k; i < numsSize; i++) {
        if (nums[i] > h[0]) { /* 比第 k 大還大，才有資格進來 */
            h[0] = nums[i];
            sift_down_min(h, k, 0);
        }
    }
    int ans = h[0];
    free(h);
    return ans;
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

    /* 隨機對照：排序後取第 k 個 */
    srand(215);
    for (int n = 1; n <= 100; n++) {
        int x[100], y[100];
        for (int i = 0; i < n; i++)
            x[i] = y[i] = rand() % 21 - 10;
        qsort(y, (size_t)n, sizeof *y, cmp_desc);
        for (int k = 1; k <= n; k++)
            assert(findKthLargest(x, n, k) == y[k - 1]);
    }
    puts("0215: passed");
    return 0;
}
