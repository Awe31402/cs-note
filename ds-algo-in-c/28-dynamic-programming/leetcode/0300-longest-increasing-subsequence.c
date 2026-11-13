/* LeetCode 300 · Longest Increasing Subsequence（嚴格遞增）
 * 思路 1（O(n²) DP）：dp[i] = 以 nums[i] 結尾的 LIS 長度 = 1 + max(dp[j]) over j < i 且 nums[j] < nums[i]。
 * 思路 2（本檔，O(n log n)）：tails[k] = 所有長度 k+1 的遞增子序列裡，最小的結尾。
 *   tails 一定是遞增的 → 對每個 x，二分找第一個 >= x 的位置換成 x（或接在最後面）。
 *   tails 的長度就是答案（但 tails 本身不一定是一個真的 LIS）。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int lengthOfLIS(int *nums, int numsSize)
{
    int *tails = malloc((size_t)numsSize * sizeof *tails), len = 0;
    for (int i = 0; i < numsSize; i++) {
        int lo = 0, hi = len;
        while (lo < hi) {
            int mid = lo + (hi - lo) / 2;
            if (tails[mid] >= nums[i]) /* >=：嚴格遞增，相等的要取代，不能接在後面 */
                hi = mid;
            else
                lo = mid + 1;
        }
        tails[lo] = nums[i];
        if (lo == len)
            len++;
    }
    free(tails);
    return len;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {10, 9, 2, 5, 3, 7, 101, 18};
    assert(lengthOfLIS(a, 8) == 4); /* 2 3 7 18 */
    int b[] = {0, 1, 0, 3, 2, 3};
    assert(lengthOfLIS(b, 6) == 4);
    int c[] = {7, 7, 7, 7};
    assert(lengthOfLIS(c, 4) == 1);
    srand(300);
    for (int t = 0; t < 500; t++) {
        int n = 1 + rand() % 40, x[40], dp[40], want = 0;
        for (int i = 0; i < n; i++) {
            x[i] = rand() % 15;
            dp[i] = 1;
            for (int j = 0; j < i; j++)
                if (x[j] < x[i] && dp[j] + 1 > dp[i])
                    dp[i] = dp[j] + 1;
            if (dp[i] > want)
                want = dp[i];
        }
        assert(lengthOfLIS(x, n) == want);
    }
    puts("0300: passed");
    return 0;
}
