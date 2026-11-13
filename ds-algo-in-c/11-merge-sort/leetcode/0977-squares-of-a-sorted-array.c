/* LeetCode 977 · Squares of a Sorted Array
 * 思路：平方後最大的一定在兩端（最負或最正）。左右兩個指標比絕對值，
 *       大的平方放到答案的尾端，往中間走。就是 merge 的反向版。O(n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
int *sortedSquares(int *nums, int numsSize, int *returnSize)
{
    int *ans = malloc((size_t)numsSize * sizeof *ans);
    int l = 0, r = numsSize - 1;
    for (int k = numsSize - 1; k >= 0; k--) {
        int a = nums[l] * nums[l], b = nums[r] * nums[r]; /* |nums| <= 10^4，平方不會溢位 */
        if (a > b) {
            ans[k] = a;
            l++;
        } else {
            ans[k] = b;
            r--;
        }
    }
    *returnSize = numsSize;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {-4, -1, 0, 3, 10}, wa[] = {0, 1, 9, 16, 100};
    int m;
    int *r = sortedSquares(a, 5, &m);
    assert(m == 5 && memcmp(r, wa, sizeof wa) == 0);
    free(r);
    int b[] = {-7, -3, 2, 3, 11}, wb[] = {4, 9, 9, 49, 121};
    r = sortedSquares(b, 5, &m);
    assert(memcmp(r, wb, sizeof wb) == 0);
    free(r);
    int c[] = {-5, -3, -2}, wc[] = {4, 9, 25};
    r = sortedSquares(c, 3, &m);
    assert(memcmp(r, wc, sizeof wc) == 0);
    free(r);
    puts("0977: passed");
    return 0;
}
