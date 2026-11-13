/* LeetCode 739 · Daily Temperatures
 * 思路：單調堆疊 (monotonic stack)。stack 裡放「還沒等到更暖日子」的 index，
 *       對應的溫度由底到頂遞減。新的一天比頂端暖 → 頂端找到答案，pop 掉，一直比下去。
 *       每個 index 最多 push、pop 各一次 → O(n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
int *dailyTemperatures(int *temperatures, int temperaturesSize, int *returnSize)
{
    int n = temperaturesSize;
    int *ans = calloc((size_t)n, sizeof *ans); /* 等不到的天數預設 0 */
    int *st = malloc((size_t)n * sizeof *st), top = 0;
    for (int i = 0; i < n; i++) {
        while (top > 0 && temperatures[i] > temperatures[st[top - 1]]) {
            int j = st[--top];
            ans[j] = i - j;
        }
        st[top++] = i;
    }
    free(st);
    *returnSize = n;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static void check(int *t, int n)
{
    int m;
    int *got = dailyTemperatures(t, n, &m);
    assert(m == n);
    for (int i = 0; i < n; i++) { /* O(n²) 暴力對照 */
        int want = 0;
        for (int j = i + 1; j < n; j++)
            if (t[j] > t[i]) {
                want = j - i;
                break;
            }
        assert(got[i] == want);
    }
    free(got);
}

int main(void)
{
    int a[] = {73, 74, 75, 71, 69, 72, 76, 73}; /* → 1 1 4 2 1 1 0 0 */
    check(a, 8);
    int b[] = {30, 40, 50, 60};
    check(b, 4);
    int c[] = {30, 60, 90};
    check(c, 3);
    int d[] = {50, 50, 50}; /* 一樣溫度不算更暖 */
    check(d, 3);
    srand(739);
    for (int t = 0; t < 300; t++) {
        int n = 1 + rand() % 50, x[50];
        for (int i = 0; i < n; i++)
            x[i] = 30 + rand() % 10;
        check(x, n);
    }
    puts("0739: passed");
    return 0;
}
