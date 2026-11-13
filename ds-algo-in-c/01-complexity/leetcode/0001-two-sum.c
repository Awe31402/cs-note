/* LeetCode 1 · Two Sum
 * 思路：把 (值, 原本 index) 依值排序，再用左右兩個指標往中間夾。
 *       暴力 O(n²) → 排序 + 雙指標 O(n log n)。
 *       用 hash table 可以做到 O(n)，第 08 主題會做。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    int val;
    int idx;
} Pair;

static int cmp_pair(const void *x, const void *y)
{
    int a = ((const Pair *)x)->val, b = ((const Pair *)y)->val;
    return (a > b) - (a < b);
}

int *twoSum(int *nums, int numsSize, int target, int *returnSize)
{
    Pair *p = malloc((size_t)numsSize * sizeof *p);
    for (int i = 0; i < numsSize; i++)
        p[i] = (Pair){nums[i], i};
    qsort(p, (size_t)numsSize, sizeof *p, cmp_pair);

    int *ans = malloc(2 * sizeof *ans);
    *returnSize = 0;
    int l = 0, r = numsSize - 1;
    while (l < r) {
        long s = (long)p[l].val + p[r].val; /* 兩個 int 相加可能溢位 */
        if (s == target) {
            ans[0] = p[l].idx;
            ans[1] = p[r].idx;
            *returnSize = 2;
            break;
        }
        if (s < target)
            l++; /* 太小：左邊換大一點的 */
        else
            r--; /* 太大：右邊換小一點的 */
    }
    free(p);
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(int *nums, int n, int target)
{
    int m;
    int *got = twoSum(nums, n, target, &m);
    assert(m == 2);
    assert(got[0] != got[1]);
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
    check(c, 2, 6); /* 同樣的值，不同 index */
    int d[] = {-1000000000, 5, 1000000000, -3};
    check(d, 4, 0);
    puts("0001: passed");
    return 0;
}
