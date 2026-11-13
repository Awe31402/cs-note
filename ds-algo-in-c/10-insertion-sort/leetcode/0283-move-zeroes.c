/* LeetCode 283 · Move Zeroes
 * 思路：把非 0 的元素依原順序往前寫（快慢指標），剩下的格子補 0。
 *       跟插入排序一樣是「穩定」的：非 0 元素相對順序不變。O(n)、O(1) 空間。
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
void moveZeroes(int *nums, int numsSize)
{
    int w = 0;
    for (int r = 0; r < numsSize; r++)
        if (nums[r] != 0)
            nums[w++] = nums[r];
    while (w < numsSize)
        nums[w++] = 0;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {0, 1, 0, 3, 12}, wa[] = {1, 3, 12, 0, 0};
    moveZeroes(a, 5);
    assert(memcmp(a, wa, sizeof a) == 0);
    int b[] = {0};
    moveZeroes(b, 1);
    assert(b[0] == 0);
    int c[] = {4, -2, 0, 0, 7, 0, -2}, wc[] = {4, -2, 7, -2, 0, 0, 0};
    moveZeroes(c, 7);
    assert(memcmp(c, wc, sizeof c) == 0);
    puts("0283: passed");
    return 0;
}
