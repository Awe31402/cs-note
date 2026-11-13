/* LeetCode 35 · Search Insert Position
 * 思路：就是 lower_bound —— 第一個 >= target 的位置。找到就是它的 index，找不到就是該插入的位置。
 */
#include <assert.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
int searchInsert(int *nums, int numsSize, int target)
{
    int lo = 0, hi = numsSize;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (nums[mid] >= target)
            hi = mid;
        else
            lo = mid + 1;
    }
    return lo;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {1, 3, 5, 6};
    assert(searchInsert(a, 4, 5) == 2);
    assert(searchInsert(a, 4, 2) == 1);
    assert(searchInsert(a, 4, 7) == 4);
    assert(searchInsert(a, 4, 0) == 0);
    int b[] = {1};
    assert(searchInsert(b, 1, 1) == 0 && searchInsert(b, 1, 2) == 1);
    puts("0035: passed");
    return 0;
}
