/* LeetCode 875 · Koko Eating Bananas
 * 思路：在「答案」上二分 (binary search on the answer)。
 *   速度 k 越大，花的小時數越少 → 「k 小時內吃得完」對 k 是 F F F T T T。
 *   找第一個 T，就是最小速度。範圍 k ∈ [1, max(piles)]。
 */
#include <assert.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
static long long hours(const int *piles, int n, int k)
{
    long long h = 0;
    for (int i = 0; i < n; i++)
        h += (piles[i] + (long long)k - 1) / k; /* 無條件進位：ceil(p / k) */
    return h;
}

int minEatingSpeed(int *piles, int pilesSize, int h)
{
    int lo = 1, hi = 1;
    for (int i = 0; i < pilesSize; i++)
        if (piles[i] > hi)
            hi = piles[i];
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (hours(piles, pilesSize, mid) <= h)
            hi = mid; /* 吃得完：答案 <= mid */
        else
            lo = mid + 1;
    }
    return lo;
}
/* ===== 提交範圍 結束 ===== */

static int brute(int *p, int n, int h)
{
    for (int k = 1;; k++)
        if (hours(p, n, k) <= h)
            return k;
}

int main(void)
{
    int a[] = {3, 6, 7, 11};
    assert(minEatingSpeed(a, 4, 8) == 4);
    int b[] = {30, 11, 23, 4, 20};
    assert(minEatingSpeed(b, 5, 5) == 30);
    assert(minEatingSpeed(b, 5, 6) == 23);
    int c[] = {1000000000};
    assert(minEatingSpeed(c, 1, 2) == 500000000);
    int d[] = {805306368, 805306368, 805306368};
    assert(minEatingSpeed(d, 3, 1000000000) == 3); /* 小時數加總要用 long long */
    for (int h = 4; h <= 40; h++)
        assert(minEatingSpeed(a, 4, h) == brute(a, 4, h));
    puts("0875: passed");
    return 0;
}
