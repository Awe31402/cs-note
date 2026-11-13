/* LeetCode 435 · Non-overlapping Intervals
 * 最少要刪掉幾個區間，剩下的才不會重疊？（[1,2] 和 [2,3] 不算重疊）
 * 思路：這就是 CLRS 15.1 的活動選擇。能「留下」的最多個數 = 依結束時間排序後，貪心挑最早結束的。
 *       答案 = 總數 − 最多能留下幾個。O(n log n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int by_end(const void *a, const void *b)
{
    int x = (*(int *const *)a)[1], y = (*(int *const *)b)[1];
    return (x > y) - (x < y);
}

int eraseOverlapIntervals(int **intervals, int intervalsSize, int *intervalsColSize)
{
    (void)intervalsColSize;
    qsort(intervals, (size_t)intervalsSize, sizeof *intervals, by_end);
    int keep = 0;
    long long last_end = -(1LL << 40);
    for (int i = 0; i < intervalsSize; i++)
        if (intervals[i][0] >= last_end) { /* 跟上一個留下的不重疊 */
            keep++;
            last_end = intervals[i][1];
        }
    return intervalsSize - keep;
}
/* ===== 提交範圍 結束 ===== */

static int run(int iv[][2], int n)
{
    int *rows[16], cols[16];
    for (int i = 0; i < n; i++) {
        rows[i] = iv[i];
        cols[i] = 2;
    }
    return eraseOverlapIntervals(rows, n, cols);
}

int main(void)
{
    int a[][2] = {{1, 2}, {2, 3}, {3, 4}, {1, 3}};
    assert(run(a, 4) == 1);
    int b[][2] = {{1, 2}, {1, 2}, {1, 2}};
    assert(run(b, 3) == 2);
    int c[][2] = {{1, 2}, {2, 3}};
    assert(run(c, 2) == 0);
    int d[][2] = {{1, 100}, {11, 22}, {1, 11}, {2, 12}}; /* 依「開始時間」貪心會留下 [1,100]，錯 */
    assert(run(d, 4) == 2);
    puts("0435: passed");
    return 0;
}
