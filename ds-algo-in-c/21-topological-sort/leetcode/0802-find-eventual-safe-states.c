/* LeetCode 802 · Find Eventual Safe States
 * 安全節點：從它出發的「每一條」路最後都會停在終點（沒有出邊的節點），不會掉進環裡。
 * 思路：三色 DFS。
 *   灰 = 正在路徑上；遇到灰 → 有環，路徑上的節點全都不安全
 *   黑 = 已確定安全
 *   一個節點安全 ⇔ 它的所有鄰居都安全。O(V + E)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
enum { WHITE, GRAY, SAFE };

static bool safe(int **g, int *deg, char *color, int u)
{
    if (color[u] != WHITE)
        return color[u] == SAFE; /* 灰：在環上（或通往環），不安全 */
    color[u] = GRAY;
    for (int i = 0; i < deg[u]; i++)
        if (!safe(g, deg, color, g[u][i]))
            return false; /* 保持灰色：之後別人走到這裡也會知道不安全 */
    color[u] = SAFE;
    return true;
}

int *eventualSafeNodes(int **graph, int graphSize, int *graphColSize, int *returnSize)
{
    char *color = calloc((size_t)graphSize, 1);
    int *ans = malloc((size_t)graphSize * sizeof *ans), k = 0;
    for (int u = 0; u < graphSize; u++) /* 由小到大檢查，答案自然就是遞增的 */
        if (safe(graph, graphColSize, color, u))
            ans[k++] = u;
    free(color);
    *returnSize = k;
    return ans;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int g0[] = {1, 2}, g1[] = {2, 3}, g2[] = {5}, g3[] = {0}, g4[] = {5};
    int *g[] = {g0, g1, g2, g3, g4, NULL, NULL}, deg[] = {2, 2, 1, 1, 1, 0, 0};
    int k, *r = eventualSafeNodes(g, 7, deg, &k);
    int want[] = {2, 4, 5, 6};
    assert(k == 4 && memcmp(r, want, sizeof want) == 0);
    free(r);

    int h0[] = {1, 2, 3, 4}, h1[] = {1, 2}, h2[] = {3, 4}, h3[] = {0, 4};
    int *h[] = {h0, h1, h2, h3, NULL}, hd[] = {4, 2, 2, 2, 0};
    r = eventualSafeNodes(h, 5, hd, &k);
    assert(k == 1 && r[0] == 4); /* 1 有自己指向自己的環 */
    free(r);
    puts("0802: passed");
    return 0;
}
