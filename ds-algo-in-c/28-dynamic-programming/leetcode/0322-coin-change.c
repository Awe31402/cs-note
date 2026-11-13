/* LeetCode 322 · Coin Change
 * 用最少枚硬幣湊出 amount（每種面額無限多），湊不出來回傳 -1。
 * 思路：dp[x] = 湊出 x 的最少枚數 = min over coin c (dp[x - c] + 1)。O(amount × 面額數)。
 *   貪心（先拿最大的）在一般面額下是錯的：coins = [1, 3, 4]、amount = 6 → 貪心 4+1+1 = 3 枚，最佳 3+3 = 2 枚。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int coinChange(int *coins, int coinsSize, int amount)
{
    int *dp = malloc(((size_t)amount + 1) * sizeof *dp);
    const int INF = amount + 1; /* 最多也只要 amount 枚（全用 1 元），比這大就是湊不出來 */
    dp[0] = 0;
    for (int x = 1; x <= amount; x++) {
        dp[x] = INF;
        for (int i = 0; i < coinsSize; i++)
            if (coins[i] <= x && dp[x - coins[i]] + 1 < dp[x])
                dp[x] = dp[x - coins[i]] + 1;
    }
    int ans = dp[amount] >= INF ? -1 : dp[amount];
    free(dp);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {1, 2, 5};
    assert(coinChange(a, 3, 11) == 3);
    int b[] = {2};
    assert(coinChange(b, 1, 3) == -1);
    int c[] = {1};
    assert(coinChange(c, 1, 0) == 0);
    int d[] = {1, 3, 4};
    assert(coinChange(d, 3, 6) == 2); /* 貪心會得到 3 */
    int e[] = {186, 419, 83, 408};
    assert(coinChange(e, 4, 6249) == 20);
    puts("0322: passed");
    return 0;
}
