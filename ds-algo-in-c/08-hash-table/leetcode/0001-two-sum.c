/* LeetCode 1 · Two Sum（Hash 版，O(n)）
 * 思路：由左往右走，對每個 x 先查「target - x」有沒有出現過；有就找到了，沒有就把 x 記下來。
 *       01 主題寫過排序 + 雙指標的 O(n log n) 版本。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
#define BITS 15 /* 2^15 = 32768 格 > 2 × 10^4，負載因子 < 0.5 */
#define CAP (1 << BITS)

static unsigned slot(int key)
{
    return ((unsigned)key * 2654435761u) >> (32 - BITS);
}

int *twoSum(int *nums, int numsSize, int target, int *returnSize)
{
    int *keys = malloc(CAP * sizeof *keys);
    int *idx = malloc(CAP * sizeof *idx);
    char *used = calloc(CAP, 1);
    int *ans = malloc(2 * sizeof *ans);
    *returnSize = 0;
    for (int i = 0; i < numsSize; i++) {
        int need = (int)((long long)target - nums[i]); /* 題目保證答案存在，need 在 int 範圍內才可能找到 */
        for (unsigned s = slot(need); used[s]; s = (s + 1) & (CAP - 1))
            if (keys[s] == need) {
                ans[0] = idx[s];
                ans[1] = i;
                *returnSize = 2;
                goto done;
            }
        unsigned s = slot(nums[i]);
        while (used[s] && keys[s] != nums[i])
            s = (s + 1) & (CAP - 1);
        used[s] = 1; /* 重複的值：保留舊的 index 也可以，這裡直接覆蓋 */
        keys[s] = nums[i];
        idx[s] = i;
    }
done:
    free(keys);
    free(idx);
    free(used);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(int *nums, int n, int target)
{
    int m;
    int *got = twoSum(nums, n, target, &m);
    assert(m == 2 && got[0] != got[1]);
    assert(nums[got[0]] + nums[got[1]] == target);
    free(got);
}

int main(void)
{
    int a[] = {2, 7, 11, 15};
    check(a, 4, 9);
    int b[] = {3, 2, 4};
    check(b, 3, 6);
    int c[] = {3, 3};
    check(c, 2, 6);
    int d[] = {-1000000000, 5, 1000000000, -3};
    check(d, 4, 0);
    static int e[10000];
    for (int i = 0; i < 10000; i++)
        e[i] = i * 7;
    check(e, 10000, 7 * 9998 + 7 * 9999);
    puts("0001: passed");
    return 0;
}
