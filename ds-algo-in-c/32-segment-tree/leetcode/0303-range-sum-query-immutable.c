/* LeetCode 303 · Range Sum Query - Immutable
 * 陣列不會變，一直問區間和。
 * 思路：前綴和。pre[i] = nums[0..i-1] 的和，sum(l, r) = pre[r+1] - pre[l]。建表 O(n)，每次查詢 O(1)。
 *       陣列不會變的時候，完全不需要線段樹 —— 這題是拿來跟 307 對照的。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    int *pre;
} NumArray;

NumArray *numArrayCreate(int *nums, int numsSize)
{
    NumArray *a = malloc(sizeof *a);
    a->pre = malloc(((size_t)numsSize + 1) * sizeof *a->pre);
    a->pre[0] = 0;
    for (int i = 0; i < numsSize; i++)
        a->pre[i + 1] = a->pre[i] + nums[i]; /* |和| <= 10^4 × 10^5 = 10^9，int 夠 */
    return a;
}

int numArraySumRange(NumArray *a, int left, int right)
{
    return a->pre[right + 1] - a->pre[left];
}

void numArrayFree(NumArray *a)
{
    free(a->pre);
    free(a);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int nums[] = {-2, 0, 3, -5, 2, -1};
    NumArray *a = numArrayCreate(nums, 6);
    assert(numArraySumRange(a, 0, 2) == 1);
    assert(numArraySumRange(a, 2, 5) == -1);
    assert(numArraySumRange(a, 0, 5) == -3);
    assert(numArraySumRange(a, 3, 3) == -5);
    numArrayFree(a);
    puts("0303: passed");
    return 0;
}
