/* LeetCode 785 · Is Graph Bipartite?
 * 二分圖 (bipartite)：頂點能分成兩群，所有邊都跨在兩群之間。匹配、最大流的很多題目都建立在二分圖上。
 * 思路：BFS 著色。起點塗 0，鄰居塗 1，鄰居的鄰居塗 0……遇到兩端同色的邊就不是二分圖。
 *       ⇔ 圖裡沒有奇數長度的環。圖可能不連通，每個元件都要檢查。O(V + E)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
bool isBipartite(int **graph, int graphSize, int *graphColSize)
{
    int n = graphSize, *color = malloc((size_t)n * sizeof *color), *q = malloc((size_t)n * sizeof *q);
    for (int i = 0; i < n; i++)
        color[i] = -1;
    bool ok = true;
    for (int s = 0; s < n && ok; s++) {
        if (color[s] != -1)
            continue;
        int head = 0, tail = 0;
        color[s] = 0;
        q[tail++] = s;
        while (head < tail && ok) {
            int u = q[head++];
            for (int i = 0; i < graphColSize[u]; i++) {
                int v = graph[u][i];
                if (color[v] == -1) {
                    color[v] = 1 - color[u];
                    q[tail++] = v;
                } else if (color[v] == color[u]) {
                    ok = false; /* 同一條邊兩端同色 → 有奇數環 */
                    break;
                }
            }
        }
    }
    free(color);
    free(q);
    return ok;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int a0[] = {1, 2, 3}, a1[] = {0, 2}, a2[] = {0, 1, 3}, a3[] = {0, 2};
    int *a[] = {a0, a1, a2, a3}, ac[] = {3, 2, 3, 2};
    assert(!isBipartite(a, 4, ac)); /* 0-1-2 是三角形 */
    int b0[] = {1, 3}, b1[] = {0, 2}, b2[] = {1, 3}, b3[] = {0, 2};
    int *b[] = {b0, b1, b2, b3}, bc[] = {2, 2, 2, 2};
    assert(isBipartite(b, 4, bc)); /* 正方形：偶數環 */
    int c1[] = {2}, c2[] = {1};
    int *c[] = {NULL, c1, c2}, cc[] = {0, 1, 1}; /* 0 是孤立點 */
    assert(isBipartite(c, 3, cc));
    puts("0785: passed");
    return 0;
}
