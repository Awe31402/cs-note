/* LeetCode 239 · Sliding Window Maximum
 * 思路：單調遞減 deque，存 index。
 *   - 新元素進來前，從尾端 pop 掉所有「比它小」的：它們永遠不會再當最大值了
 *   - 頭端的 index 如果已經滑出視窗，就從頭 pop 掉
 *   - 頭端就是目前視窗的最大值
 *   每個 index 最多進出 deque 各一次 → O(n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
int *maxSlidingWindow(int *nums, int numsSize, int k, int *returnSize)
{
    int *dq = malloc((size_t)numsSize * sizeof *dq); /* 最多 n 個，用陣列當 deque，head/tail 只會往右 */
    int head = 0, tail = 0;
    int *ans = malloc((size_t)(numsSize - k + 1) * sizeof *ans), m = 0;
    for (int i = 0; i < numsSize; i++) {
        while (tail > head && nums[dq[tail - 1]] <= nums[i])
            tail--;
        dq[tail++] = i;
        if (dq[head] <= i - k) /* 滑出視窗 */
            head++;
        if (i >= k - 1)
            ans[m++] = nums[dq[head]];
    }
    free(dq);
    *returnSize = m;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(int *a, int n, int k)
{
    int m;
    int *got = maxSlidingWindow(a, n, k, &m);
    assert(m == n - k + 1);
    for (int i = 0; i + k <= n; i++) { /* O(nk) 暴力對照 */
        int mx = a[i];
        for (int j = i; j < i + k; j++)
            if (a[j] > mx)
                mx = a[j];
        assert(got[i] == mx);
    }
    free(got);
}

int main(void)
{
    int a[] = {1, 3, -1, -3, 5, 3, 6, 7}; /* → 3 3 5 5 6 7 */
    check(a, 8, 3);
    int b[] = {1};
    check(b, 1, 1);
    int c[] = {9, 8, 7, 6, 5};
    check(c, 5, 2);
    srand(239);
    for (int t = 0; t < 500; t++) {
        int n = 1 + rand() % 40, x[40];
        for (int i = 0; i < n; i++)
            x[i] = rand() % 10;
        check(x, n, 1 + rand() % n);
    }
    puts("0239: passed");
    return 0;
}
