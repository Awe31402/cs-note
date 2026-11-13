/* LeetCode 238 · Product of Array Except Self（不能用除法）
 * 思路：answer[i] = (i 左邊全部的乘積) × (i 右邊全部的乘積)。
 *       第一趟由左往右放「左乘積」，第二趟由右往左乘上「右乘積」。O(n)，除了答案外 O(1) 空間。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int *productExceptSelf(int *nums, int numsSize, int *returnSize)
{
    int *ans = malloc((size_t)numsSize * sizeof *ans);
    int left = 1;
    for (int i = 0; i < numsSize; i++) {
        ans[i] = left; /* nums[0..i-1] 的乘積 */
        left *= nums[i];
    }
    int right = 1;
    for (int i = numsSize - 1; i >= 0; i--) {
        ans[i] *= right; /* 再乘上 nums[i+1..n-1] 的乘積 */
        right *= nums[i];
    }
    *returnSize = numsSize;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(int *nums, int n)
{
    int m;
    int *got = productExceptSelf(nums, n, &m);
    assert(m == n);
    for (int i = 0; i < n; i++) {
        int p = 1;
        for (int j = 0; j < n; j++)
            if (j != i)
                p *= nums[j];
        assert(got[i] == p);
    }
    free(got);
}

int main(void)
{
    int a[] = {1, 2, 3, 4}; /* → 24 12 8 6 */
    check(a, 4);
    int b[] = {-1, 1, 0, -3, 3}; /* 有一個 0 */
    check(b, 5);
    int c[] = {0, 4, 0}; /* 兩個 0：全部都是 0 */
    check(c, 3);
    srand(238);
    for (int t = 0; t < 300; t++) {
        int n = 2 + rand() % 8, x[10];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 7 - 3;
        check(x, n);
    }
    puts("0238: passed");
    return 0;
}
