/* LeetCode 1697 · Checking Existence of Edge Length Limited Paths
 * 每個查詢 (p, q, limit)：p 到 q 有沒有一條路，路上每條邊都 < limit？
 * 思路：離線處理 (offline) + Kruskal 的想法。
 *   1. 邊依權重排序；查詢依 limit 排序（記住原本的 index）
 *   2. 由小到大處理查詢：把所有權重 < limit 的邊 union 進來，再看 p、q 是否同一組
 *   邊只會一路加進來、不會刪 → Union-Find 剛好適用。O((E + Q) log + (E + Q) α(n))。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
static int find(int *p, int x)
{
    while (p[x] != x) {
        p[x] = p[p[x]];
        x = p[x];
    }
    return x;
}

static int cmp_by_w(const void *a, const void *b) /* 元素是 int*，比第 3 個欄位 */
{
    int x = (*(int *const *)a)[2], y = (*(int *const *)b)[2];
    return (x > y) - (x < y);
}

bool *distanceLimitedPathsExist(int n, int **edgeList, int edgeListSize, int *edgeListColSize, int **queries,
                                int queriesSize, int *queriesColSize, int *returnSize)
{
    (void)edgeListColSize;
    (void)queriesColSize;
    int **edges = malloc((size_t)(edgeListSize ? edgeListSize : 1) * sizeof *edges);
    for (int i = 0; i < edgeListSize; i++)
        edges[i] = edgeList[i];
    qsort(edges, (size_t)edgeListSize, sizeof *edges, cmp_by_w);

    /* 查詢排序時要記住原本的位置：用 {p, q, limit, 原 index} */
    int (*qs)[4] = malloc((size_t)queriesSize * sizeof *qs);
    int **qp = malloc((size_t)queriesSize * sizeof *qp);
    for (int i = 0; i < queriesSize; i++) {
        qs[i][0] = queries[i][0];
        qs[i][1] = queries[i][1];
        qs[i][2] = queries[i][2];
        qs[i][3] = i;
        qp[i] = qs[i];
    }
    qsort(qp, (size_t)queriesSize, sizeof *qp, cmp_by_w);

    int *p = malloc((size_t)n * sizeof *p);
    for (int i = 0; i < n; i++)
        p[i] = i;
    bool *ans = malloc((size_t)queriesSize * sizeof *ans);
    int e = 0;
    for (int i = 0; i < queriesSize; i++) {
        int limit = qp[i][2];
        while (e < edgeListSize && edges[e][2] < limit) { /* 嚴格小於 */
            p[find(p, edges[e][0])] = find(p, edges[e][1]);
            e++;
        }
        ans[qp[i][3]] = find(p, qp[i][0]) == find(p, qp[i][1]);
    }
    free(edges);
    free(qs);
    free(qp);
    free(p);
    *returnSize = queriesSize;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int e0[] = {0, 1, 2}, e1[] = {1, 2, 4}, e2[] = {2, 0, 8}, e3[] = {1, 0, 16};
    int *edges[] = {e0, e1, e2, e3}, ec[] = {3, 3, 3, 3};
    int q0[] = {0, 1, 2}, q1[] = {0, 2, 5};
    int *qs[] = {q0, q1}, qc[] = {3, 3}, k;
    bool *r = distanceLimitedPathsExist(3, edges, 4, ec, qs, 2, qc, &k);
    assert(k == 2 && !r[0] && r[1]); /* 0-1 的邊權重是 2，不 < 2 */
    free(r);

    int f0[] = {0, 1, 10}, f1[] = {1, 2, 5}, f2[] = {2, 3, 9}, f3[] = {3, 4, 13};
    int *edges2[] = {f0, f1, f2, f3};
    int p0[] = {0, 4, 14}, p1[] = {1, 4, 13};
    int *qs2[] = {p0, p1};
    r = distanceLimitedPathsExist(5, edges2, 4, ec, qs2, 2, qc, &k);
    assert(r[0] && !r[1]);
    free(r);
    puts("1697: passed");
    return 0;
}
