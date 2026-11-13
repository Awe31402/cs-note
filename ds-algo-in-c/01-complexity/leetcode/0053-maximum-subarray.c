/* LeetCode 53 · Maximum Subarray
 * 思路：Kadane 演算法，O(n)。
 *   cur  = 「以 i 結尾」的最大子陣列和
 *   cur  = max(nums[i], cur + nums[i])  ← 前面的和是負的就丟掉，從 i 重新開始
 *   best = 所有 cur 裡最大的
 * 暴力：O(n³) 三層迴圈 → O(n²) 固定起點往右加 → O(n) Kadane。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int maxSubArray(int *nums, int numsSize)
{
    int cur = nums[0], best = nums[0];
    for (int i = 1; i < numsSize; i++) {
        cur = cur > 0 ? cur + nums[i] : nums[i];
        if (cur > best)
            best = cur;
    }
    return best;
}
/* ===== 提交範圍 結束 ===== */

/* O(n²)：固定起點 i，往右累加 */
static int brute(const int *a, int n)
{
    int best = a[0];
    for (int i = 0; i < n; i++) {
        int s = 0;
        for (int j = i; j < n; j++) {
            s += a[j];
            if (s > best)
                best = s;
        }
    }
    return best;
}

int main(void)
{
    int a[] = {-2, 1, -3, 4, -1, 2, 1, -5, 4};
    assert(maxSubArray(a, 9) == 6); /* [4,-1,2,1] */
    int b[] = {1};
    assert(maxSubArray(b, 1) == 1);
    int c[] = {5, 4, -1, 7, 8};
    assert(maxSubArray(c, 5) == 23);
    int d[] = {-3, -1, -2}; /* 全負：答案是最大的那個，不是 0 */
    assert(maxSubArray(d, 3) == -1);

    srand(53);
    for (int n = 1; n <= 200; n++) {
        int x[200];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 21 - 10;
        assert(maxSubArray(x, n) == brute(x, n));
    }
    puts("0053: passed");
    return 0;
}
