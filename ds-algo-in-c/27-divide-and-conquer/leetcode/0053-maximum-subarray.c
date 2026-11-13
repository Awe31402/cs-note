/* LeetCode 53 · Maximum Subarray（分治版）
 * 01 主題用 Kadane 做過 O(n)。這裡是題目「Follow up」要的分治版（CLRS 第 3 版 4.1）：
 *   答案只有三種：全在左半、全在右半、跨過中點。跨中點的 = 從中點往左最大延伸 + 往右最大延伸。
 *   T(n) = 2T(n/2) + O(n) = O(n log n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int best(const int *a, int lo, int hi)
{
    if (lo == hi)
        return a[lo];
    int mid = lo + (hi - lo) / 2;
    int left = best(a, lo, mid), right = best(a, mid + 1, hi);
    int s = 0, lmax = a[mid], rmax = a[mid + 1];
    for (int i = mid; i >= lo; i--) { /* 一定要包含 a[mid] */
        s += a[i];
        if (s > lmax)
            lmax = s;
    }
    s = 0;
    for (int j = mid + 1; j <= hi; j++) { /* 一定要包含 a[mid+1] */
        s += a[j];
        if (s > rmax)
            rmax = s;
    }
    int cross = lmax + rmax, m = left > right ? left : right;
    return m > cross ? m : cross;
}

int maxSubArray(int *nums, int numsSize)
{
    return best(nums, 0, numsSize - 1);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {-2, 1, -3, 4, -1, 2, 1, -5, 4};
    assert(maxSubArray(a, 9) == 6);
    int b[] = {5, 4, -1, 7, 8};
    assert(maxSubArray(b, 5) == 23);
    int c[] = {-3, -1, -2};
    assert(maxSubArray(c, 3) == -1);
    srand(530);
    for (int t = 0; t < 1000; t++) { /* 對照 Kadane */
        int n = 1 + rand() % 50, x[50];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 21 - 10;
        int cur = x[0], want = x[0];
        for (int i = 1; i < n; i++) {
            cur = cur > 0 ? cur + x[i] : x[i];
            if (cur > want)
                want = cur;
        }
        assert(maxSubArray(x, n) == want);
    }
    puts("0053: passed");
    return 0;
}
