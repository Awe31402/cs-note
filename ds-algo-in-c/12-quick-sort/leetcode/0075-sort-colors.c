/* LeetCode 75 · Sort Colors（只有 0、1、2，一趟、原地）
 * 思路：荷蘭國旗問題 = 以 1 為 pivot 的三路切分。
 *   [0, lo)   都是 0
 *   [lo, i)   都是 1
 *   [i, hi]   還沒看
 *   (hi, n)   都是 2
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
void sortColors(int *nums, int numsSize)
{
    int lo = 0, i = 0, hi = numsSize - 1;
    while (i <= hi) {
        if (nums[i] == 0) {
            int t = nums[lo];
            nums[lo++] = nums[i];
            nums[i++] = t; /* 換過來的一定是 1（或 i == lo），可以直接往前 */
        } else if (nums[i] == 2) {
            int t = nums[hi];
            nums[hi--] = nums[i];
            nums[i] = t; /* 換過來的還沒看過，i 不能動 */
        } else {
            i++;
        }
    }
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    srand(75);
    for (int t = 0; t < 1000; t++) {
        int n = 1 + rand() % 40, a[40], cnt[3] = {0};
        for (int i = 0; i < n; i++)
            cnt[a[i] = rand() % 3]++;
        sortColors(a, n);
        int k = 0;
        for (int c = 0; c < 3; c++)
            for (int j = 0; j < cnt[c]; j++)
                assert(a[k++] == c);
    }
    puts("0075: passed");
    return 0;
}
