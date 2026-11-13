/* LeetCode 976 · Largest Perimeter Triangle
 * 思路：排序後由大到小看連續三個 a <= b <= c：只要 a + b > c 就能圍成三角形，而且周長最大。
 *       如果連最大的 a、b 都不夠，c 就不可能當最長邊，換下一組。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b); /* 不寫 a - b：可能溢位 */
}

int largestPerimeter(int *nums, int numsSize)
{
    qsort(nums, (size_t)numsSize, sizeof *nums, cmp_int);
    for (int i = numsSize - 1; i >= 2; i--)
        if (nums[i - 2] + nums[i - 1] > nums[i])
            return nums[i - 2] + nums[i - 1] + nums[i];
    return 0;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {2, 1, 2};
    assert(largestPerimeter(a, 3) == 5);
    int b[] = {1, 2, 1, 10};
    assert(largestPerimeter(b, 4) == 0);
    int c[] = {3, 6, 2, 3};
    assert(largestPerimeter(c, 4) == 8);
    srand(976);
    for (int t = 0; t < 300; t++) { /* 暴力三層迴圈對照 */
        int n = 3 + rand() % 12, x[15];
        for (int i = 0; i < n; i++)
            x[i] = 1 + rand() % 20;
        int best = 0;
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++)
                for (int k = j + 1; k < n; k++) {
                    int p = x[i], q = x[j], r = x[k];
                    if (p + q > r && p + r > q && q + r > p && p + q + r > best)
                        best = p + q + r;
                }
        assert(largestPerimeter(x, n) == best);
    }
    puts("0976: passed");
    return 0;
}
