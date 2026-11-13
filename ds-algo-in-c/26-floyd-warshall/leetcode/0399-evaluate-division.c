/* LeetCode 399 · Evaluate Division
 * 給 a / b = 2.0、b / c = 3.0 這類等式，回答 a / c = ? 之類的查詢；算不出來回傳 -1.0。
 * 思路：變數是頂點，a / b = k 就是邊 a → b 權重 k、b → a 權重 1/k。
 *   a / c = (a / b) × (b / c)：沿著路徑把權重「相乘」。
 *   變數最多 40 個 → Floyd-Warshall（把「相加取最小」換成「相乘、有路就填」），O(V³)。
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
static int id_of(char names[][6], int *n, const char *s, int create)
{
    for (int i = 0; i < *n; i++)
        if (strcmp(names[i], s) == 0)
            return i;
    if (!create)
        return -1;
    strcpy(names[*n], s); /* 題目：變數名稱長度 1..5 */
    return (*n)++;
}

double *calcEquation(char ***equations, int equationsSize, int *equationsColSize, double *values, int valuesSize,
                     char ***queries, int queriesSize, int *queriesColSize, int *returnSize)
{
    (void)equationsColSize;
    (void)valuesSize;
    (void)queriesColSize;
    char names[40][6];
    int n = 0;
    double r[40][40]; /* r[i][j] = i / j；0 代表還不知道 */
    memset(r, 0, sizeof r);
    for (int i = 0; i < equationsSize; i++) {
        int a = id_of(names, &n, equations[i][0], 1), b = id_of(names, &n, equations[i][1], 1);
        r[a][b] = values[i];
        r[b][a] = 1.0 / values[i];
    }
    for (int i = 0; i < n; i++)
        r[i][i] = 1.0;
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++)
            if (r[i][k] != 0)
                for (int j = 0; j < n; j++)
                    if (r[k][j] != 0 && r[i][j] == 0)
                        r[i][j] = r[i][k] * r[k][j]; /* 題目保證不矛盾，任何一條路算出來都一樣 */
    double *ans = malloc((size_t)queriesSize * sizeof *ans);
    for (int i = 0; i < queriesSize; i++) {
        int a = id_of(names, &n, queries[i][0], 0), b = id_of(names, &n, queries[i][1], 0);
        ans[i] = (a < 0 || b < 0 || r[a][b] == 0) ? -1.0 : r[a][b];
    }
    *returnSize = queriesSize;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    char *e0[] = {"a", "b"}, *e1[] = {"b", "c"};
    char **eq[] = {e0, e1};
    int ec[] = {2, 2};
    double vals[] = {2.0, 3.0};
    char *q0[] = {"a", "c"}, *q1[] = {"b", "a"}, *q2[] = {"a", "e"}, *q3[] = {"a", "a"}, *q4[] = {"x", "x"};
    char **qs[] = {q0, q1, q2, q3, q4};
    int qc[] = {2, 2, 2, 2, 2}, k;
    double *r = calcEquation(eq, 2, ec, vals, 2, qs, 5, qc, &k);
    double want[] = {6.0, 0.5, -1.0, 1.0, -1.0}; /* x 沒出現過：連 x / x 都算不出來 */
    assert(k == 5);
    for (int i = 0; i < 5; i++)
        assert(fabs(r[i] - want[i]) < 1e-9);
    free(r);

    char *f0[] = {"a", "b"}, *f1[] = {"c", "d"}; /* 兩群不相連 */
    char **eq2[] = {f0, f1};
    char *p0[] = {"a", "d"};
    char **qs2[] = {p0};
    r = calcEquation(eq2, 2, ec, vals, 2, qs2, 1, qc, &k);
    assert(r[0] == -1.0);
    free(r);
    puts("0399: passed");
    return 0;
}
