/* LeetCode 55 · Jump Game
 * 站在 i 最多可以往前跳 nums[i] 格。從 0 出發，跳得到最後一格嗎？
 * 思路：維護「目前最遠能到哪裡」reach。掃過每一格：這格到得了（i <= reach），就用它更新 reach。
 *       O(n)、O(1)。不需要 DP 去記每一格能不能到。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
bool canJump(int *nums, int numsSize)
{
    int reach = 0;
    for (int i = 0; i < numsSize; i++) {
        if (i > reach)
            return false; /* 這格根本到不了，後面更不可能 */
        if (i + nums[i] > reach)
            reach = i + nums[i];
        if (reach >= numsSize - 1)
            return true;
    }
    return true;
}
/* ===== 提交範圍 結束 ===== */

static bool brute(const int *a, int n) /* DP：ok[i] = 能不能到 i */
{
    bool ok[40] = {true};
    for (int i = 0; i < n; i++)
        if (ok[i])
            for (int j = 1; j <= a[i] && i + j < n; j++)
                ok[i + j] = true;
    return ok[n - 1];
}

int main(void)
{
    int a[] = {2, 3, 1, 1, 4};
    assert(canJump(a, 5));
    int b[] = {3, 2, 1, 0, 4}; /* 每條路都卡在 index 3 */
    assert(!canJump(b, 5));
    int c[] = {0};
    assert(canJump(c, 1));
    srand(55);
    for (int t = 0; t < 1000; t++) {
        int n = 1 + rand() % 40, x[40];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 3;
        assert(canJump(x, n) == brute(x, n));
    }
    puts("0055: passed");
    return 0;
}
