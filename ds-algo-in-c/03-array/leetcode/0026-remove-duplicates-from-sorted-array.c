/* LeetCode 26 · Remove Duplicates from Sorted Array
 * 思路：快慢指標 (two pointers)。slow 指向「下一個不重複的值要放的位置」，
 *       fast 往前掃，遇到跟前一個不同的值就寫到 slow。O(n)、O(1) 空間。
 */
#include <assert.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
int removeDuplicates(int *nums, int numsSize)
{
    if (numsSize == 0)
        return 0;
    int slow = 1;
    for (int fast = 1; fast < numsSize; fast++)
        if (nums[fast] != nums[slow - 1])
            nums[slow++] = nums[fast];
    return slow;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {1, 1, 2};
    assert(removeDuplicates(a, 3) == 2 && a[0] == 1 && a[1] == 2);

    int b[] = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4}, wb[] = {0, 1, 2, 3, 4};
    int k = removeDuplicates(b, 10);
    assert(k == 5);
    for (int i = 0; i < k; i++)
        assert(b[i] == wb[i]);

    int c[] = {-3, -3, -3};
    assert(removeDuplicates(c, 3) == 1 && c[0] == -3);
    int d[] = {7};
    assert(removeDuplicates(d, 1) == 1);
    puts("0026: passed");
    return 0;
}
