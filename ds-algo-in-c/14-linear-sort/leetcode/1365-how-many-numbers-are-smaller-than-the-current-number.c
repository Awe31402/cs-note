/* LeetCode 1365 · How Many Numbers Are Smaller Than the Current Number（0 <= nums[i] <= 100）
 * 思路：計數排序的前兩步。cnt[v] = v 出現幾次；前綴和後 less[v] = 比 v 小的個數。O(n + 101)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
int *smallerNumbersThanCurrent(int *nums, int numsSize, int *returnSize)
{
    int cnt[102] = {0}; /* 多一格：cnt[v + 1] 存 v 的次數，前綴和後 cnt[v] 就是「< v」的個數 */
    for (int i = 0; i < numsSize; i++)
        cnt[nums[i] + 1]++;
    for (int v = 1; v <= 101; v++)
        cnt[v] += cnt[v - 1];
    int *ans = malloc((size_t)numsSize * sizeof *ans);
    for (int i = 0; i < numsSize; i++)
        ans[i] = cnt[nums[i]];
    *returnSize = numsSize;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {8, 1, 2, 2, 3}, wa[] = {4, 0, 1, 1, 3}, m;
    int *r = smallerNumbersThanCurrent(a, 5, &m);
    assert(m == 5 && memcmp(r, wa, sizeof wa) == 0);
    free(r);
    int b[] = {7, 7, 7, 7}, wb[] = {0, 0, 0, 0};
    r = smallerNumbersThanCurrent(b, 4, &m);
    assert(memcmp(r, wb, sizeof wb) == 0);
    free(r);
    srand(1365);
    for (int t = 0; t < 300; t++) {
        int n = 2 + rand() % 50, x[52];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 101;
        r = smallerNumbersThanCurrent(x, n, &m);
        for (int i = 0; i < n; i++) {
            int c = 0;
            for (int j = 0; j < n; j++)
                c += x[j] < x[i];
            assert(r[i] == c);
        }
        free(r);
    }
    puts("1365: passed");
    return 0;
}
