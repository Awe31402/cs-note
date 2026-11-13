/* LeetCode 1480 · Running Sum of 1d Array
 * 思路：前綴和 (prefix sum)。out[i] = out[i-1] + nums[i]，O(n)。
 *       暴力解每個位置都從頭加，是 O(n²)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int *runningSum(int *nums, int numsSize, int *returnSize)
{
    int *out = malloc((size_t)numsSize * sizeof *out);
    for (int i = 0; i < numsSize; i++)
        out[i] = nums[i] + (i > 0 ? out[i - 1] : 0);
    *returnSize = numsSize;
    return out;
}
/* ===== 提交範圍 結束 ===== */

/* O(n²) 暴力解，只拿來對照 */
static int brute_at(const int *nums, int i)
{
    int s = 0;
    for (int j = 0; j <= i; j++)
        s += nums[j];
    return s;
}

int main(void)
{
    int a[] = {1, 2, 3, 4};
    int want[] = {1, 3, 6, 10};
    int m;
    int *got = runningSum(a, 4, &m);
    assert(m == 4);
    for (int i = 0; i < 4; i++)
        assert(got[i] == want[i]);
    free(got);

    srand(1480);
    int b[1000];
    for (int i = 0; i < 1000; i++)
        b[i] = rand() % 2001 - 1000;
    got = runningSum(b, 1000, &m);
    for (int i = 0; i < 1000; i++)
        assert(got[i] == brute_at(b, i));
    free(got);

    puts("1480: passed");
    return 0;
}
