/* LeetCode 1462 · Course Schedule IV
 * prerequisites[i] = [a, b]：a 是 b 的先修課（直接）。先修關係有遞移性。
 * 每個查詢 [u, v]：u 是不是 v 的先修課（直接或間接）？
 * 思路：Warshall 遞移閉包，n <= 100，O(n³)。之後每個查詢 O(1)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
bool *checkIfPrerequisite(int numCourses, int **prerequisites, int prerequisitesSize, int *prerequisitesColSize,
                          int **queries, int queriesSize, int *queriesColSize, int *returnSize)
{
    (void)prerequisitesColSize;
    (void)queriesColSize;
    int n = numCourses;
    bool r[100][100] = {{false}};
    for (int i = 0; i < prerequisitesSize; i++)
        r[prerequisites[i][0]][prerequisites[i][1]] = true;
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++)
            if (r[i][k])
                for (int j = 0; j < n; j++)
                    if (r[k][j])
                        r[i][j] = true;
    bool *ans = malloc((size_t)queriesSize * sizeof *ans);
    for (int i = 0; i < queriesSize; i++)
        ans[i] = r[queries[i][0]][queries[i][1]];
    *returnSize = queriesSize;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int p0[] = {1, 0};
    int *pre[] = {p0}, pc[] = {2};
    int q0[] = {0, 1}, q1[] = {1, 0};
    int *qs[] = {q0, q1}, qc[] = {2, 2}, k;
    bool *r = checkIfPrerequisite(2, pre, 1, pc, qs, 2, qc, &k);
    assert(k == 2 && !r[0] && r[1]);
    free(r);

    int a0[] = {1, 2}, a1[] = {1, 0}, a2[] = {2, 0};
    int *pre2[] = {a0, a1, a2}, pc2[] = {2, 2, 2};
    int b0[] = {1, 0}, b1[] = {1, 2};
    int *qs2[] = {b0, b1};
    r = checkIfPrerequisite(3, pre2, 3, pc2, qs2, 2, qc, &k);
    assert(r[0] && r[1]);
    free(r);

    int c0[] = {0, 1}, c1[] = {1, 2}, c2[] = {2, 3}; /* 間接：0 → 1 → 2 → 3 */
    int *pre3[] = {c0, c1, c2};
    int d0[] = {0, 3}, d1[] = {3, 0};
    int *qs3[] = {d0, d1};
    r = checkIfPrerequisite(4, pre3, 3, pc2, qs3, 2, qc, &k);
    assert(r[0] && !r[1]);
    free(r);
    puts("1462: passed");
    return 0;
}
