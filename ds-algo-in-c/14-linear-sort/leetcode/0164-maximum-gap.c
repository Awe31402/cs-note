/* LeetCode 164 · Maximum Gap（要求線性時間與空間）
 * 思路：桶排序 + 鴿籠原理。n 個數分布在 [lo, hi]，最大間距至少是 (hi - lo) / (n - 1)。
 *       把桶寬設成這個值（至少 1），同一個桶裡的兩個數差距一定小於桶寬，
 *       所以最大間距只會出現在「相鄰兩個非空桶」之間：後一桶的最小值 - 前一桶的最大值。
 *       每個桶只要記 min 和 max，不用真的排序。O(n)。
 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int maximumGap(int *nums, int numsSize)
{
    int n = numsSize;
    if (n < 2)
        return 0;
    int lo = nums[0], hi = nums[0];
    for (int i = 1; i < n; i++) {
        if (nums[i] < lo)
            lo = nums[i];
        if (nums[i] > hi)
            hi = nums[i];
    }
    if (lo == hi)
        return 0;
    long long width = ((long long)hi - lo) / (n - 1);
    if (width < 1)
        width = 1;
    int nb = (int)(((long long)hi - lo) / width) + 1;
    int *bmin = malloc((size_t)nb * sizeof *bmin), *bmax = malloc((size_t)nb * sizeof *bmax);
    for (int b = 0; b < nb; b++) {
        bmin[b] = INT_MAX;
        bmax[b] = INT_MIN; /* 空桶 */
    }
    for (int i = 0; i < n; i++) {
        int b = (int)(((long long)nums[i] - lo) / width);
        if (nums[i] < bmin[b])
            bmin[b] = nums[i];
        if (nums[i] > bmax[b])
            bmax[b] = nums[i];
    }
    int best = 0, prev_max = bmax[0]; /* 桶 0 一定有 lo */
    for (int b = 1; b < nb; b++) {
        if (bmax[b] == INT_MIN)
            continue; /* 跳過空桶 */
        if (bmin[b] - prev_max > best)
            best = bmin[b] - prev_max;
        prev_max = bmax[b];
    }
    free(bmin);
    free(bmax);
    return best;
}
/* ===== 提交範圍 結束 ===== */

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

int main(void)
{
    int a[] = {3, 6, 9, 1};
    assert(maximumGap(a, 4) == 3);
    int b[] = {10};
    assert(maximumGap(b, 1) == 0);
    int c[] = {1, 1, 1, 1};
    assert(maximumGap(c, 4) == 0);
    int d[] = {0, 1000000000};
    assert(maximumGap(d, 2) == 1000000000);
    srand(164);
    for (int t = 0; t < 1000; t++) {
        int n = 2 + rand() % 50, x[52], y[52];
        for (int i = 0; i < n; i++)
            x[i] = y[i] = rand() % (t % 2 ? 30 : 1000000);
        qsort(y, (size_t)n, sizeof *y, cmp_int);
        int want = 0;
        for (int i = 1; i < n; i++)
            if (y[i] - y[i - 1] > want)
                want = y[i] - y[i - 1];
        assert(maximumGap(x, n) == want);
    }
    puts("0164: passed");
    return 0;
}
