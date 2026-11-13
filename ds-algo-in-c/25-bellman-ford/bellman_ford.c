#include "bellman_ford.h"

#include <stdlib.h>

int bellman_ford(int n, const BEdge *e, int m, int s, long long *dist, int *parent)
{
    for (int v = 0; v < n; v++) {
        dist[v] = BF_INF;
        parent[v] = -1;
    }
    dist[s] = 0;
    /* 最短路徑最多 n-1 條邊；第 i 輪結束後，「最多用 i 條邊」的最短路徑都正確了 */
    for (int round = 1; round < n; round++) {
        int changed = 0;
        for (int i = 0; i < m; i++) {
            if (dist[e[i].u] == BF_INF)
                continue; /* 還走不到 u：∞ + 負數 不能當成更短 */
            long long nd = dist[e[i].u] + e[i].w;
            if (nd < dist[e[i].v]) {
                dist[e[i].v] = nd;
                parent[e[i].v] = e[i].u;
                changed = 1;
            }
        }
        if (!changed)
            break; /* 提早結束：這一輪都沒變，之後也不會變 */
    }
    for (int i = 0; i < m; i++) /* 第 n 輪還能鬆弛 → 有負環 */
        if (dist[e[i].u] != BF_INF && dist[e[i].u] + e[i].w < dist[e[i].v])
            return 0;
    return 1;
}

int find_negative_cycle(int n, const BEdge *e, int m, int *cycle)
{
    /* 想像有一個虛擬起點連到每個頂點（權重 0）：所有 dist 從 0 開始 */
    long long *dist = calloc((size_t)(n ? n : 1), sizeof *dist);
    int *parent = malloc((size_t)(n ? n : 1) * sizeof *parent);
    if (!dist || !parent)
        abort();
    for (int v = 0; v < n; v++)
        parent[v] = -1;
    int last = -1;
    for (int round = 0; round < n; round++) { /* 第 n 輪（round = n-1）還有變動 → 有負環 */
        last = -1;
        for (int i = 0; i < m; i++)
            if (dist[e[i].u] + e[i].w < dist[e[i].v]) {
                dist[e[i].v] = dist[e[i].u] + e[i].w;
                parent[e[i].v] = e[i].u;
                last = e[i].v;
            }
    }
    int k = 0;
    if (last != -1) {
        /* last 可能只是「被負環影響」而不在環上；沿 parent 往回走 n 步，一定會進到環裡 */
        int v = last;
        for (int i = 0; i < n; i++)
            v = parent[v];
        for (int u = v;; u = parent[u]) {
            cycle[k++] = u;
            if (u == v && k > 1)
                break;
        }
        k--; /* 最後一個是重複的 v */
        for (int l = 0, r = k - 1; l < r; l++, r--) { /* parent 是反方向，翻回來 */
            int t = cycle[l];
            cycle[l] = cycle[r];
            cycle[r] = t;
        }
    }
    free(dist);
    free(parent);
    return k;
}

int dag_shortest_paths(int n, const BEdge *e, int m, int s, long long *dist, int *parent)
{
    /* Kahn 拓撲排序 */
    int *indeg = calloc((size_t)(n ? n : 1), sizeof *indeg), *order = malloc((size_t)(n ? n : 1) * sizeof *order);
    int *start = calloc((size_t)n + 1, sizeof *start), *idx = malloc((size_t)(m ? m : 1) * sizeof *idx);
    int *fill = calloc((size_t)(n ? n : 1), sizeof *fill);
    if (!indeg || !order || !start || !idx || !fill)
        abort();
    for (int i = 0; i < m; i++) {
        indeg[e[i].v]++;
        start[e[i].u + 1]++;
    }
    for (int v = 0; v < n; v++)
        start[v + 1] += start[v];
    for (int i = 0; i < m; i++)
        idx[start[e[i].u] + fill[e[i].u]++] = i; /* 每個頂點的出邊 index */
    int head = 0, tail = 0;
    for (int v = 0; v < n; v++)
        if (indeg[v] == 0)
            order[tail++] = v;
    while (head < tail) {
        int u = order[head++];
        for (int k = start[u]; k < start[u + 1]; k++)
            if (--indeg[e[idx[k]].v] == 0)
                order[tail++] = e[idx[k]].v;
    }
    int ok = tail == n;
    if (ok) {
        for (int v = 0; v < n; v++) {
            dist[v] = BF_INF;
            parent[v] = -1;
        }
        dist[s] = 0;
        for (int i = 0; i < n; i++) { /* 依拓撲順序：輪到 u 時，所有能到 u 的頂點都處理過了 */
            int u = order[i];
            if (dist[u] == BF_INF)
                continue;
            for (int k = start[u]; k < start[u + 1]; k++) {
                const BEdge *x = &e[idx[k]];
                if (dist[u] + x->w < dist[x->v]) {
                    dist[x->v] = dist[u] + x->w;
                    parent[x->v] = u;
                }
            }
        }
    }
    free(indeg);
    free(order);
    free(start);
    free(idx);
    free(fill);
    return ok;
}
