/* LeetCode 33 · Search in Rotated Sorted Array（值都不相同）
 * 思路：從 mid 切開，左右兩半「至少有一半是排好序的」。
 *   - 判斷哪一半有序（比 nums[lo] 和 nums[mid]）
 *   - target 落在有序那半的範圍內 → 往那半找；否則往另一半找
 */
#include <assert.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
int search(int *nums, int numsSize, int target)
{
    int lo = 0, hi = numsSize - 1; /* 這題用閉區間比較好寫：要讀 nums[hi] */
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (nums[mid] == target)
            return mid;
        if (nums[lo] <= nums[mid]) { /* 左半 [lo, mid] 有序；<= 處理 lo == mid */
            if (nums[lo] <= target && target < nums[mid])
                hi = mid - 1;
            else
                lo = mid + 1;
        } else { /* 右半 [mid, hi] 有序 */
            if (nums[mid] < target && target <= nums[hi])
                lo = mid + 1;
            else
                hi = mid - 1;
        }
    }
    return -1;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {4, 5, 6, 7, 0, 1, 2};
    assert(search(a, 7, 0) == 4);
    assert(search(a, 7, 3) == -1);
    int b[] = {1};
    assert(search(b, 1, 0) == -1 && search(b, 1, 1) == 0);
    /* 所有旋轉量 × 所有目標 */
    for (int n = 1; n <= 12; n++)
        for (int r = 0; r < n; r++) {
            int x[12];
            for (int i = 0; i < n; i++)
                x[i] = ((i + r) % n) * 2; /* 偶數，奇數拿來測找不到 */
            for (int t = -1; t <= 2 * n; t++) {
                int got = search(x, n, t);
                if (t % 2 == 0 && t >= 0 && t < 2 * n)
                    assert(got >= 0 && x[got] == t);
                else
                    assert(got == -1);
            }
        }
    puts("0033: passed");
    return 0;
}
