/* LeetCode 34 · Find First and Last Position of Element in Sorted Array
 * 思路：兩次二分。
 *   first = lower_bound(target)        第一個 >= target
 *   last  = lower_bound(target + 1) - 1  （等於 upper_bound(target) - 1）
 *   first 超出範圍或 nums[first] != target → 不存在。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int lower_bound(const int *a, int n, long long x) /* long long：target + 1 可能溢位 */
{
    int lo = 0, hi = n;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] >= x)
            hi = mid;
        else
            lo = mid + 1;
    }
    return lo;
}

int *searchRange(int *nums, int numsSize, int target, int *returnSize)
{
    int *ans = malloc(2 * sizeof *ans);
    int first = lower_bound(nums, numsSize, target);
    if (first == numsSize || nums[first] != target) {
        ans[0] = ans[1] = -1;
    } else {
        ans[0] = first;
        ans[1] = lower_bound(nums, numsSize, (long long)target + 1) - 1;
    }
    *returnSize = 2;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(int *a, int n, int t, int w0, int w1)
{
    int m;
    int *r = searchRange(a, n, t, &m);
    assert(m == 2 && r[0] == w0 && r[1] == w1);
    free(r);
}

int main(void)
{
    int a[] = {5, 7, 7, 8, 8, 10};
    check(a, 6, 8, 3, 4);
    check(a, 6, 6, -1, -1);
    check(a, 6, 5, 0, 0);
    check(a, 6, 10, 5, 5);
    check(NULL, 0, 0, -1, -1);
    int b[] = {2147483647, 2147483647};
    check(b, 2, 2147483647, 0, 1); /* target + 1 溢位的情況 */
    puts("0034: passed");
    return 0;
}
