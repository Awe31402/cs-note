/* LeetCode 189 · Rotate Array
 * 思路：三次反轉。往右轉 k 格 = 整個反轉 → 前 k 個反轉 → 後 n-k 個反轉。O(n)、O(1) 空間。
 *   [1 2 3 4 5 6 7], k=3
 *   整個反轉   [7 6 5 4 3 2 1]
 *   前 3 反轉  [5 6 7 | 4 3 2 1]
 *   後 4 反轉  [5 6 7 | 1 2 3 4]
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
static void reverse(int *a, int l, int r)
{
    while (l < r) {
        int t = a[l];
        a[l++] = a[r];
        a[r--] = t;
    }
}

void rotate(int *nums, int numsSize, int k)
{
    k %= numsSize; /* k 可能比 n 大 */
    reverse(nums, 0, numsSize - 1);
    reverse(nums, 0, k - 1);
    reverse(nums, k, numsSize - 1);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {1, 2, 3, 4, 5, 6, 7}, wa[] = {5, 6, 7, 1, 2, 3, 4};
    rotate(a, 7, 3);
    assert(memcmp(a, wa, sizeof a) == 0);

    int b[] = {-1, -100, 3, 99}, wb[] = {3, 99, -1, -100};
    rotate(b, 4, 2);
    assert(memcmp(b, wb, sizeof b) == 0);

    /* 隨機：跟「用額外陣列」的直覺解對照 */
    srand(189);
    for (int n = 1; n <= 30; n++)
        for (int k = 0; k <= 70; k++) {
            int x[30], want[30];
            for (int i = 0; i < n; i++)
                x[i] = rand() % 100;
            for (int i = 0; i < n; i++)
                want[(i + k) % n] = x[i];
            rotate(x, n, k);
            assert(memcmp(x, want, (size_t)n * sizeof *x) == 0);
        }
    puts("0189: passed");
    return 0;
}
