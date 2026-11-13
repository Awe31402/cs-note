#include "topo.h"

#include <stdlib.h>

static void *xcalloc(size_t n, size_t sz)
{
    void *p = calloc(n ? n : 1, sz);
    if (!p)
        abort();
    return p;
}

void digraph_build(Digraph *g, int n, const int (*edges)[2], int m)
{
    g->n = n;
    g->start = xcalloc((size_t)n + 1, sizeof *g->start);
    g->adj = xcalloc((size_t)m, sizeof *g->adj);
    for (int i = 0; i < m; i++)
        g->start[edges[i][0] + 1]++; /* 先數每個頂點的出度 */
    for (int u = 0; u < n; u++)
        g->start[u + 1] += g->start[u]; /* 前綴和 = 每段的起點 */
    int *fill = xcalloc((size_t)n, sizeof *fill);
    for (int i = 0; i < m; i++) {
        int u = edges[i][0];
        g->adj[g->start[u] + fill[u]++] = edges[i][1];
    }
    free(fill);
}

void digraph_free(Digraph *g)
{
    free(g->start);
    free(g->adj);
    g->n = 0;
}

int topo_kahn(const Digraph *g, int *order)
{
    int n = g->n, head = 0, tail = 0;
    int *indeg = xcalloc((size_t)n, sizeof *indeg);
    for (int i = 0; i < g->start[n]; i++)
        indeg[g->adj[i]]++;
    for (int v = 0; v < n; v++)
        if (indeg[v] == 0)
            order[tail++] = v; /* order 本身就當 queue 用 */
    while (head < tail) {
        int u = order[head++];
        for (int i = g->start[u]; i < g->start[u + 1]; i++)
            if (--indeg[g->adj[i]] == 0) /* 所有前置條件都完成了 */
                order[tail++] = g->adj[i];
    }
    free(indeg);
    return tail; /* 有環的話，環上的頂點入度永遠不會變 0 */
}

enum { WHITE, GRAY, BLACK };

/* 回傳 1 = 發現環。color：白 = 沒走過、灰 = 在目前的遞迴路徑上、黑 = 已完成 */
static int visit(const Digraph *g, int u, char *color, int *order, int *k, int *parent, int *cyc_end, int *cyc_start)
{
    color[u] = GRAY;
    for (int i = g->start[u]; i < g->start[u + 1]; i++) {
        int v = g->adj[i];
        if (color[v] == GRAY) { /* 後向邊 (back edge) → 有環 */
            *cyc_start = v;
            *cyc_end = u;
            return 1;
        }
        if (color[v] == WHITE) {
            parent[v] = u;
            if (visit(g, v, color, order, k, parent, cyc_end, cyc_start))
                return 1;
        }
    }
    color[u] = BLACK;
    order[--*k] = u; /* 完成時放到最前面：完成越晚排越前面 */
    return 0;
}

static int run_dfs(const Digraph *g, int *order, int *cycle)
{
    int n = g->n, k = n, cyc_end = -1, cyc_start = -1, found = 0;
    char *color = xcalloc((size_t)n, 1);
    int *parent = xcalloc((size_t)n, sizeof *parent);
    int *tmp = order ? order : xcalloc((size_t)n, sizeof *tmp);
    for (int u = 0; u < n && !found; u++)
        if (color[u] == WHITE)
            found = visit(g, u, color, tmp, &k, parent, &cyc_end, &cyc_start);
    int len = 0;
    if (found && cycle) { /* 從 cyc_end 沿 parent 往回走到 cyc_start */
        for (int v = cyc_end; v != cyc_start; v = parent[v])
            cycle[len++] = v;
        cycle[len++] = cyc_start;
        for (int l = 0, r = len - 1; l < r; l++, r--) {
            int t = cycle[l];
            cycle[l] = cycle[r];
            cycle[r] = t;
        }
    }
    free(color);
    free(parent);
    if (!order)
        free(tmp);
    return found ? len : -1;
}

int topo_dfs(const Digraph *g, int *order)
{
    return run_dfs(g, order, NULL) < 0; /* -1 = 沒有環 */
}

int find_cycle(const Digraph *g, int *cycle)
{
    int len = run_dfs(g, NULL, cycle);
    return len < 0 ? 0 : len;
}
