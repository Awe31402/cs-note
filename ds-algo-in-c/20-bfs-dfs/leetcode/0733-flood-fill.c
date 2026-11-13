/* LeetCode 733 · Flood Fill
 * 思路：從 (sr, sc) 出發 DFS，把跟起點同色、上下左右相連的格子全部塗成新顏色。
 *       新顏色跟原本一樣就直接回傳（否則會無限遞迴）。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static void fill(int **img, int m, int n, int r, int c, int old, int color)
{
    if (r < 0 || r >= m || c < 0 || c >= n || img[r][c] != old)
        return;
    img[r][c] = color; /* 塗了就不會再等於 old，等於順便標記成「已拜訪」 */
    fill(img, m, n, r + 1, c, old, color);
    fill(img, m, n, r - 1, c, old, color);
    fill(img, m, n, r, c + 1, old, color);
    fill(img, m, n, r, c - 1, old, color);
}

int **floodFill(int **image, int imageSize, int *imageColSize, int sr, int sc, int color,
                int *returnSize, int **returnColumnSizes)
{
    int old = image[sr][sc];
    if (old != color)
        fill(image, imageSize, imageColSize[0], sr, sc, old, color);
    *returnSize = imageSize;
    *returnColumnSizes = malloc((size_t)imageSize * sizeof **returnColumnSizes);
    for (int i = 0; i < imageSize; i++)
        (*returnColumnSizes)[i] = imageColSize[i];
    return image; /* 原地修改 */
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int r0[] = {1, 1, 1}, r1[] = {1, 1, 0}, r2[] = {1, 0, 1};
    int *img[] = {r0, r1, r2}, cols[] = {3, 3, 3}, m, *rc;
    floodFill(img, 3, cols, 1, 1, 2, &m, &rc);
    int want[3][3] = {{2, 2, 2}, {2, 2, 0}, {2, 0, 1}}; /* 右下角的 1 沒有相連 */
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            assert(img[i][j] == want[i][j]);
    free(rc);

    int s0[] = {0, 0, 0}, s1[] = {0, 0, 0};
    int *img2[] = {s0, s1};
    floodFill(img2, 2, cols, 0, 0, 0, &m, &rc); /* 同顏色：不能無限遞迴 */
    assert(s0[0] == 0 && s1[2] == 0);
    free(rc);
    puts("0733: passed");
    return 0;
}
