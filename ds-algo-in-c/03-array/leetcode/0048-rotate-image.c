/* LeetCode 48 · Rotate Image（n×n 矩陣順時針轉 90 度，原地）
 * 思路：順時針 90° = 先轉置 (transpose)，再把每一列左右翻轉。
 *   1 2 3      1 4 7      7 4 1
 *   4 5 6  →   2 5 8  →   8 5 2
 *   7 8 9      3 6 9      9 6 3
 */
#include <assert.h>
#include <stdio.h>

/* ===== 提交範圍 開始 ===== */
void rotate(int **matrix, int matrixSize, int *matrixColSize)
{
    (void)matrixColSize;
    int n = matrixSize;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++) { /* 只換上三角，不然會換回去 */
            int t = matrix[i][j];
            matrix[i][j] = matrix[j][i];
            matrix[j][i] = t;
        }
    for (int i = 0; i < n; i++)
        for (int l = 0, r = n - 1; l < r; l++, r--) {
            int t = matrix[i][l];
            matrix[i][l] = matrix[i][r];
            matrix[i][r] = t;
        }
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    for (int n = 1; n <= 6; n++) {
        int buf[6][6], orig[6][6], *rows[6], col = n;
        for (int i = 0; i < n; i++) {
            rows[i] = buf[i];
            for (int j = 0; j < n; j++)
                orig[i][j] = buf[i][j] = i * n + j;
        }
        rotate(rows, n, &col);
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                assert(buf[i][j] == orig[n - 1 - j][i]); /* 旋轉的定義 */
    }
    puts("0048: passed");
    return 0;
}
