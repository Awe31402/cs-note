/* LeetCode 452 · Minimum Number of Arrows to Burst Balloons
 * 每個氣球是一段區間 [x_start, x_end]，一支垂直射出的箭可以射破所有「包含它的 x」的氣球。最少要幾支箭？
 * 思路：依右端點排序。第一支箭射在第一個氣球的右端點（盡量往右，才能多射到後面的）；
 *       之後遇到左端點 > 這支箭位置的氣球，才需要新的一支箭，射在它的右端點。O(n log n)。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int by_end(const void *a, const void *b)
{
    int x = (*(int *const *)a)[1], y = (*(int *const *)b)[1];
    return (x > y) - (x < y); /* 不能寫 x - y：座標範圍是整個 int，相減會溢位 */
}

int findMinArrowShots(int **points, int pointsSize, int *pointsColSize)
{
    (void)pointsColSize;
    qsort(points, (size_t)pointsSize, sizeof *points, by_end);
    int arrows = 1, pos = points[0][1];
    for (int i = 1; i < pointsSize; i++)
        if (points[i][0] > pos) { /* 這支箭射不到：左端點在箭的右邊（邊界相等算射得到） */
            arrows++;
            pos = points[i][1];
        }
    return arrows;
}
/* ===== 提交範圍 結束 ===== */

static int run(int p[][2], int n)
{
    int *rows[16], cols[16];
    for (int i = 0; i < n; i++) {
        rows[i] = p[i];
        cols[i] = 2;
    }
    return findMinArrowShots(rows, n, cols);
}

int main(void)
{
    int a[][2] = {{10, 16}, {2, 8}, {1, 6}, {7, 12}};
    assert(run(a, 4) == 2);
    int b[][2] = {{1, 2}, {3, 4}, {5, 6}, {7, 8}};
    assert(run(b, 4) == 4);
    int c[][2] = {{1, 2}, {2, 3}, {3, 4}, {4, 5}};
    assert(run(c, 4) == 2);
    int d[][2] = {{-2147483647 - 1, 2147483647}, {-2147483647 - 1, -2147483647 - 1}, {2147483647, 2147483647}};
    assert(run(d, 3) == 2);
    puts("0452: passed");
    return 0;
}
