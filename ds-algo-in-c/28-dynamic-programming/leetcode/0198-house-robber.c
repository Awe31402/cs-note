/* LeetCode 198 · House Robber
 * 一排房子，不能偷相鄰的兩間，最多能偷多少？
 * 思路：dp[i] = 前 i 間最多能偷多少 = max(dp[i-1]（不偷第 i 間）, dp[i-2] + nums[i]（偷第 i 間）)。
 *       只需要前兩項。O(n)、O(1)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int rob(int *nums, int numsSize)
{
    int skip = 0, take = 0; /* 到上一間為止：「上一間沒偷」「上一間有偷」的最大值 */
    for (int i = 0; i < numsSize; i++) {
        int best_prev = skip > take ? skip : take;
        take = skip + nums[i]; /* 偷這間 → 上一間一定沒偷 */
        skip = best_prev;      /* 不偷這間 → 上一間偷不偷都可以 */
    }
    return skip > take ? skip : take;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {1, 2, 3, 1};
    assert(rob(a, 4) == 4);
    int b[] = {2, 7, 9, 3, 1};
    assert(rob(b, 5) == 12);
    int c[] = {2, 1, 1, 2}; /* 偷頭尾：不一定是隔一間偷一間 */
    assert(rob(c, 4) == 4);
    srand(198);
    for (int t = 0; t < 500; t++) { /* 枚舉所有「不相鄰」的選法 */
        int n = 1 + rand() % 15, x[15];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 50;
        int best = 0;
        for (int mask = 0; mask < 1 << n; mask++) {
            if (mask & (mask >> 1))
                continue;
            int s = 0;
            for (int i = 0; i < n; i++)
                if (mask >> i & 1)
                    s += x[i];
            if (s > best)
                best = s;
        }
        assert(rob(x, n) == best);
    }
    puts("0198: passed");
    return 0;
}
