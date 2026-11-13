#include "graph.h"

#include <stdlib.h>

void graph_init(Graph *g, int n)
{
    g->n = n;
    g->adj = calloc((size_t)n, sizeof *g->adj);
    g->deg = calloc((size_t)n, sizeof *g->deg);
    g->cap = calloc((size_t)n, sizeof *g->cap);
    if (n > 0 && (!g->adj || !g->deg || !g->cap))
        abort();
}

void graph_free(Graph *g)
{
    for (int u = 0; u < g->n; u++)
        free(g->adj[u]);
    free(g->adj);
    free(g->deg);
    free(g->cap);
    g->n = 0;
}

void graph_add_edge(Graph *g, int u, int v)
{
    if (g->deg[u] == g->cap[u]) {
        g->cap[u] = g->cap[u] ? g->cap[u] * 2 : 4;
        g->adj[u] = realloc(g->adj[u], (size_t)g->cap[u] * sizeof **g->adj);
        if (!g->adj[u])
            abort();
    }
    g->adj[u][g->deg[u]++] = v;
}

void graph_add_undirected(Graph *g, int u, int v)
{
    graph_add_edge(g, u, v);
    graph_add_edge(g, v, u);
}

int bfs(const Graph *g, int s, int *dist, int *parent)
{
    int *q = malloc((size_t)(g->n > 0 ? g->n : 1) * sizeof *q), head = 0, tail = 0;
    for (int v = 0; v < g->n; v++) {
        dist[v] = -1; /* -1 = 還沒發現（CLRS 的白色） */
        parent[v] = -1;
    }
    dist[s] = 0;
    q[tail++] = s;
    while (head < tail) {
        int u = q[head++];
        for (int i = 0; i < g->deg[u]; i++) {
            int v = g->adj[u][i];
            if (dist[v] == -1) { /* 第一次看到就設好距離，之後不會再更小 */
                dist[v] = dist[u] + 1;
                parent[v] = u;
                q[tail++] = v;
            }
        }
    }
    free(q);
    return tail;
}

int build_path(const int *parent, int s, int t, int *path)
{
    int len = 0;
    for (int v = t; v != -1; v = parent[v])
        path[len++] = v; /* 從 t 往回走到 s */
    if (path[len - 1] != s)
        return 0;
    for (int l = 0, r = len - 1; l < r; l++, r--) {
        int tmp = path[l];
        path[l] = path[r];
        path[r] = tmp;
    }
    return len;
}

static void dfs_visit(const Graph *g, int u, int *d, int *f, int *parent, int *time)
{
    d[u] = ++*time; /* 變灰：發現 */
    for (int i = 0; i < g->deg[u]; i++) {
        int v = g->adj[u][i];
        if (d[v] == 0) {
            parent[v] = u;
            dfs_visit(g, v, d, f, parent, time);
        }
    }
    f[u] = ++*time; /* 變黑：所有鄰居都處理完 */
}

void dfs(const Graph *g, int *d, int *f, int *parent)
{
    int time = 0;
    for (int u = 0; u < g->n; u++) {
        d[u] = f[u] = 0;
        parent[u] = -1;
    }
    for (int u = 0; u < g->n; u++)
        if (d[u] == 0) /* 圖可能不連通：每個還沒走過的都當起點 */
            dfs_visit(g, u, d, f, parent, &time);
}

int dfs_iter(const Graph *g, int s, int *order)
{
    /* stack 裡放 (頂點, 下一個要看的鄰居 index)，就能模擬遞迴的順序 */
    int *st_v = malloc((size_t)g->n * sizeof *st_v), *st_i = malloc((size_t)g->n * sizeof *st_i);
    char *seen = calloc((size_t)g->n, 1);
    int top = 0, k = 0;
    st_v[top] = s;
    st_i[top++] = 0;
    seen[s] = 1;
    order[k++] = s;
    while (top > 0) {
        int u = st_v[top - 1];
        if (st_i[top - 1] == g->deg[u]) {
            top--; /* u 的鄰居都看完了：回溯 */
            continue;
        }
        int v = g->adj[u][st_i[top - 1]++];
        if (!seen[v]) {
            seen[v] = 1;
            order[k++] = v;
            st_v[top] = v;
            st_i[top++] = 0;
        }
    }
    free(st_v);
    free(st_i);
    free(seen);
    return k;
}

int connected_components(const Graph *g, int *comp)
{
    int *dist = malloc((size_t)g->n * sizeof *dist), *parent = malloc((size_t)g->n * sizeof *parent);
    int c = 0;
    for (int v = 0; v < g->n; v++)
        comp[v] = -1;
    for (int s = 0; s < g->n; s++) {
        if (comp[s] != -1)
            continue;
        bfs(g, s, dist, parent);
        for (int v = 0; v < g->n; v++)
            if (dist[v] >= 0)
                comp[v] = c;
        c++;
    }
    free(dist);
    free(parent);
    return c;
}
