/* LeetCode 905 · Sort Array By Parity
 * 思路：就是 quicksort 的 partition，條件換成「是不是偶數」。
 *       Hoare 式左右夾：左邊找奇數、右邊找偶數，交換。O(n)、O(1) 空間。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int *sortArrayByParity(int *nums, int numsSize, int *returnSize)
{
    int l = 0, r = numsSize - 1;
    while (l < r) {
        if (nums[l] % 2 == 0) {
            l++;
        } else if (nums[r] % 2 == 1) {
            r--;
        } else { /* 左奇右偶：交換 */
            int t = nums[l];
            nums[l++] = nums[r];
            nums[r--] = t;
        }
    }
    *returnSize = numsSize;
    return nums;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    srand(905);
    for (int t = 0; t < 500; t++) {
        int n = 1 + rand() % 30, a[30], cnt[11] = {0}, m;
        for (int i = 0; i < n; i++)
            cnt[a[i] = rand() % 11]++;
        int *r = sortArrayByParity(a, n, &m);
        assert(m == n);
        int seen_odd = 0;
        for (int i = 0; i < n; i++) {
            if (r[i] % 2)
                seen_odd = 1;
            else
                assert(!seen_odd); /* 偶數都在奇數前面 */
            cnt[r[i]]--;
        }
        for (int v = 0; v <= 10; v++)
            assert(cnt[v] == 0); /* 元素沒有少也沒有多 */
    }
    puts("0905: passed");
    return 0;
}
