/* LeetCode 704 · Binary Search
 * 思路：標準二分搜尋，左閉右開 [lo, hi)。
 */
#include <assert.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
int search(int *nums, int numsSize, int target)
{
    int lo = 0, hi = numsSize;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (nums[mid] == target)
            return mid;
        if (nums[mid] < target)
            lo = mid + 1;
        else
            hi = mid;
    }
    return -1;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {-1, 0, 3, 5, 9, 12};
    assert(search(a, 6, 9) == 4);
    assert(search(a, 6, 2) == -1);
    for (int i = 0; i < 6; i++)
        assert(search(a, 6, a[i]) == i);
    assert(search(a, 6, -5) == -1 && search(a, 6, 13) == -1);
    int b[] = {5};
    assert(search(b, 1, 5) == 0 && search(b, 1, 4) == -1);
    puts("0704: passed");
    return 0;
}
