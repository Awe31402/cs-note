/* LeetCode 169 · Majority Element（出現超過 n/2 次的元素，保證存在）
 * 思路（分治）：左半的多數元素 l、右半的多數元素 r。整段的多數元素一定是 l 或 r 其中之一：
 *   如果 x 在兩半都沒過半，那加起來也不會過半。l == r 就是答案；不同就各數一次看誰多。
 *   T(n) = 2T(n/2) + O(n) = O(n log n)。
 *   面試最佳解是 Boyer-Moore 投票法：O(n)、O(1) 空間，見 README。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int count_in(const int *a, int lo, int hi, int x)
{
    int c = 0;
    for (int i = lo; i <= hi; i++)
        c += a[i] == x;
    return c;
}

static int maj(const int *a, int lo, int hi)
{
    if (lo == hi)
        return a[lo];
    int mid = lo + (hi - lo) / 2;
    int l = maj(a, lo, mid), r = maj(a, mid + 1, hi);
    if (l == r)
        return l;
    return count_in(a, lo, hi, l) > count_in(a, lo, hi, r) ? l : r;
}

int majorityElement(int *nums, int numsSize)
{
    return maj(nums, 0, numsSize - 1);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a[] = {3, 2, 3};
    assert(majorityElement(a, 3) == 3);
    int b[] = {2, 2, 1, 1, 1, 2, 2};
    assert(majorityElement(b, 7) == 2);
    int c[] = {-1000000000};
    assert(majorityElement(c, 1) == -1000000000);
    srand(169);
    for (int t = 0; t < 1000; t++) {
        int n = 1 + rand() % 60, x[60], m = rand() % 3;
        for (int i = 0; i < n; i++)
            x[i] = i <= n / 2 ? m : rand() % 3; /* 前 n/2+1 個一定是 m */
        for (int i = n - 1; i > 0; i--) {
            int j = rand() % (i + 1), tmp = x[i];
            x[i] = x[j];
            x[j] = tmp;
        }
        assert(majorityElement(x, n) == m);
    }
    puts("0169: passed");
    return 0;
}
