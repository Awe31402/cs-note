/* LeetCode 698 · Partition to K Equal Sum Subsets（n <= 16）
 * 把陣列分成 k 組、每組總和一樣。這是 NP-complete 問題（k = 2 就是 PARTITION，SUBSET-SUM 的特例）。
 * 思路：n 很小 → 位元 DP (bitmask DP)。
 *   目標每組 target = 總和 / k。dp[mask] = 用掉 mask 這些數字之後，「目前這一組」已經裝了多少（-1 = 做不到）。
 *   用掉的數字依序填進一組一組，裝滿 target 就開下一組 → 目前這組的量 = sum(mask) % target。
 *   轉移：加一個還沒用的數字 x，只要不超過這一組剩下的空間。O(2^n · n)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
bool canPartitionKSubsets(int *nums, int numsSize, int k)
{
    int n = numsSize, sum = 0;
    for (int i = 0; i < n; i++)
        sum += nums[i];
    if (sum % k)
        return false;
    int target = sum / k;
    for (int i = 0; i < n; i++)
        if (nums[i] > target)
            return false;
    int full = 1 << n;
    int *dp = malloc((size_t)full * sizeof *dp);
    for (int m = 0; m < full; m++)
        dp[m] = -1;
    dp[0] = 0;
    for (int mask = 0; mask < full; mask++) {
        if (dp[mask] < 0)
            continue;
        for (int i = 0; i < n; i++) {
            if (mask >> i & 1)
                continue;
            if (dp[mask] + nums[i] > target)
                continue; /* 目前這組裝不下 */
            int next = mask | 1 << i;
            if (dp[next] < 0)
                dp[next] = (dp[mask] + nums[i]) % target; /* 剛好裝滿 → 變 0，開新的一組 */
        }
    }
    bool ok = dp[full - 1] == 0;
    free(dp);
    return ok;
}
/* ===== 提交範圍 結束 ===== */

/* 暴力：每個數字都試著放進 k 組中的任一組（k^n） */
static bool brute(const int *a, int n, int k, int *load, int i, int target)
{
    if (i == n) {
        for (int j = 0; j < k; j++)
            if (load[j] != target)
                return false;
        return true;
    }
    for (int j = 0; j < k; j++) {
        load[j] += a[i];
        bool ok = load[j] <= target && brute(a, n, k, load, i + 1, target);
        load[j] -= a[i];
        if (ok)
            return true;
    }
    return false;
}

int main(void)
{
    int a[] = {4, 3, 2, 3, 5, 2, 1};
    assert(canPartitionKSubsets(a, 7, 4)); /* (5) (1,4) (2,3) (2,3) */
    int b[] = {1, 2, 3, 4};
    assert(!canPartitionKSubsets(b, 4, 3));
    srand(698);
    for (int t = 0; t < 500; t++) {
        int n = 1 + rand() % 8, k = 1 + rand() % 4, x[8], sum = 0;
        for (int i = 0; i < n; i++)
            sum += x[i] = 1 + rand() % 6;
        bool want = false;
        if (sum % k == 0) {
            int load[4] = {0};
            want = brute(x, n, k, load, 0, sum / k);
        }
        assert(canPartitionKSubsets(x, n, k) == want);
    }
    puts("0698: passed");
    return 0;
}
