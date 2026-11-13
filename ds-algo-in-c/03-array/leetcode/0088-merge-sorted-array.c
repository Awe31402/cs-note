/* LeetCode 88 · Merge Sorted Array
 * 思路：nums1 後面有 n 格空位。從「後面」開始放最大的，就不會蓋掉還沒看的元素。O(m + n)，不用額外空間。
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
void merge(int *nums1, int nums1Size, int m, int *nums2, int nums2Size, int n)
{
    (void)nums1Size;
    (void)nums2Size;
    int i = m - 1, j = n - 1, k = m + n - 1;
    while (j >= 0) { /* nums2 放完就結束；nums1 剩下的本來就在正確位置 */
        if (i >= 0 && nums1[i] > nums2[j])
            nums1[k--] = nums1[i--];
        else
            nums1[k--] = nums2[j--];
    }
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {1, 2, 3, 0, 0, 0}, b[] = {2, 5, 6}, wa[] = {1, 2, 2, 3, 5, 6};
    merge(a, 6, 3, b, 3, 3);
    assert(memcmp(a, wa, sizeof a) == 0);

    int c[] = {1}, wc[] = {1};
    merge(c, 1, 1, NULL, 0, 0);
    assert(memcmp(c, wc, sizeof c) == 0);

    int d[] = {0}, e[] = {1};
    merge(d, 1, 0, e, 1, 1); /* m = 0 */
    assert(d[0] == 1);

    int f[] = {4, 5, 6, 0, 0, 0}, g[] = {1, 2, 3}, wf[] = {1, 2, 3, 4, 5, 6};
    merge(f, 6, 3, g, 3, 3);
    assert(memcmp(f, wf, sizeof f) == 0);
    puts("0088: passed");
    return 0;
}
