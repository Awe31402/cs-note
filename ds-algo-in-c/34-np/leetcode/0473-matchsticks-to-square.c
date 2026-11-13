/* LeetCode 473 · Matchsticks to Square（n <= 15）
 * 火柴全部用上、不能折斷，能不能圍成一個正方形？= 分成 4 組、每組總和一樣（698 的 k = 4）。
 * 思路：回溯 (backtracking) + 剪枝 (pruning)，示範另一種面對 NP-hard 問題的做法：
 *   1. 總和不是 4 的倍數、或有火柴比邊長還長 → 直接 false
 *   2. 由長到短放：長的火柴選擇少，先放能更早發現走不通
 *   3. 兩個邊目前長度一樣時，放哪個都等價 → 只試第一個（去掉對稱的重複搜尋）
 *   最壞還是指數時間，但剪枝讓實際上非常快。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int cmp_desc(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x < y) - (x > y);
}

static bool place(const int *a, int n, int i, int side[4], int target)
{
    if (i == n)
        return true; /* 每根都放了、而且沒有超過 → 四邊剛好都是 target */
    for (int j = 0; j < 4; j++) {
        if (side[j] + a[i] > target)
            continue;
        bool same = false;
        for (int p = 0; p < j && !same; p++)
            same = side[p] == side[j]; /* 跟前面某個邊一樣長：試過了，跳過 */
        if (same)
            continue;
        side[j] += a[i];
        if (place(a, n, i + 1, side, target))
            return true;
        side[j] -= a[i];
    }
    return false;
}

bool makesquare(int *matchsticks, int matchsticksSize)
{
    long long sum = 0;
    for (int i = 0; i < matchsticksSize; i++)
        sum += matchsticks[i];
    if (sum % 4)
        return false;
    int target = (int)(sum / 4);
    qsort(matchsticks, (size_t)matchsticksSize, sizeof *matchsticks, cmp_desc);
    if (matchsticks[0] > target)
        return false;
    int side[4] = {0};
    return place(matchsticks, matchsticksSize, 0, side, target);
}
/* ===== 提交範圍 結束 ===== */

static bool brute(const int *a, int n) /* 4^n 種分法，只適用很小的 n */
{
    int sum = 0;
    for (int i = 0; i < n; i++)
        sum += a[i];
    if (sum % 4)
        return false;
    int total = 1;
    for (int i = 0; i < n; i++)
        total *= 4;
    for (int code = 0; code < total; code++) {
        int s[4] = {0}, c = code;
        for (int i = 0; i < n; i++, c /= 4)
            s[c % 4] += a[i];
        if (s[0] == s[1] && s[1] == s[2] && s[2] == s[3])
            return true;
    }
    return false;
}

int main(void)
{
    int a[] = {1, 1, 2, 2, 2};
    assert(makesquare(a, 5));
    int b[] = {3, 3, 3, 3, 4};
    assert(!makesquare(b, 5));
    int c[] = {5, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3};
    assert(makesquare(c, 12));
    srand(473);
    for (int t = 0; t < 500; t++) {
        int n = 1 + rand() % 8, x[8], y[8];
        for (int i = 0; i < n; i++)
            x[i] = y[i] = 1 + rand() % 5;
        assert(makesquare(x, n) == brute(y, n));
    }
    puts("0473: passed");
    return 0;
}
