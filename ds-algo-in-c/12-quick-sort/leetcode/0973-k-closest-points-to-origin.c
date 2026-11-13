/* LeetCode 973 · K Closest Points to Origin
 * 思路：quickselect，以「距離平方」比大小，把最近的 k 個放到陣列前 k 格。平均 O(n)。
 *       不用開根號：比大小時平方就夠了。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int dist(int *p)
{
    return p[0] * p[0] + p[1] * p[1]; /* |x|,|y| <= 10^4 → 最多 2×10^8，int 夠 */
}

static void swap(int **a, int **b)
{
    int *t = *a;
    *a = *b;
    *b = t;
}

int **kClosest(int **points, int pointsSize, int *pointsColSize, int k, int *returnSize, int **returnColumnSizes)
{
    (void)pointsColSize;
    int lo = 0, hi = pointsSize - 1, target = k - 1; /* 要讓 index k-1 就定位 */
    while (lo < hi) {
        swap(&points[hi], &points[lo + rand() % (hi - lo + 1)]);
        int pd = dist(points[hi]), i = lo - 1;
        for (int j = lo; j < hi; j++) /* Lomuto partition，只交換指標 */
            if (dist(points[j]) <= pd)
                swap(&points[++i], &points[j]);
        swap(&points[i + 1], &points[hi]);
        int p = i + 1;
        if (p == target)
            break;
        if (target < p)
            hi = p - 1;
        else
            lo = p + 1;
    }
    int **ans = malloc((size_t)k * sizeof *ans);
    *returnColumnSizes = malloc((size_t)k * sizeof **returnColumnSizes);
    for (int i = 0; i < k; i++) {
        ans[i] = malloc(2 * sizeof **ans);
        ans[i][0] = points[i][0];
        ans[i][1] = points[i][1];
        (*returnColumnSizes)[i] = 2;
    }
    *returnSize = k;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

int main(void)
{
    srand(973);
    for (int t = 0; t < 500; t++) {
        int n = 1 + rand() % 40, buf[40][2], *pts[40], col[40], d[40];
        for (int i = 0; i < n; i++) {
            buf[i][0] = rand() % 21 - 10;
            buf[i][1] = rand() % 21 - 10;
            pts[i] = buf[i];
            col[i] = 2;
            d[i] = dist(buf[i]);
        }
        qsort(d, (size_t)n, sizeof *d, cmp_int);
        int k = 1 + rand() % n, m, *cols;
        int **r = kClosest(pts, n, col, k, &m, &cols);
        assert(m == k);
        /* 答案的距離多重集合 = 排序後前 k 個距離 */
        int got[40];
        for (int i = 0; i < k; i++)
            got[i] = dist(r[i]);
        qsort(got, (size_t)k, sizeof *got, cmp_int);
        for (int i = 0; i < k; i++)
            assert(got[i] == d[i] && cols[i] == 2);
        for (int i = 0; i < k; i++)
            free(r[i]);
        free(r);
        free(cols);
    }
    puts("0973: passed");
    return 0;
}
