#include "dijkstra.h"

#include <stdlib.h>

static void *xcalloc(size_t n, size_t sz)
{
    void *p = calloc(n ? n : 1, sz);
    if (!p)
        abort();
    return p;
}

void wgraph_build(WGraph *g, int n, const int (*edges)[3], int m)
{
    g->n = n;
    g->start = xcalloc((size_t)n + 1, sizeof *g->start);
    g->to = xcalloc((size_t)m, sizeof *g->to);
    g->w = xcalloc((size_t)m, sizeof *g->w);
    int *fill = xcalloc((size_t)n, sizeof *fill);
    for (int i = 0; i < m; i++)
        g->start[edges[i][0] + 1]++;
    for (int u = 0; u < n; u++)
        g->start[u + 1] += g->start[u];
    for (int i = 0; i < m; i++) {
        int u = edges[i][0], k = g->start[u] + fill[u]++;
        g->to[k] = edges[i][1];
        g->w[k] = edges[i][2];
    }
    free(fill);
}

void wgraph_free(WGraph *g)
{
    free(g->start);
    free(g->to);
    free(g->w);
    g->n = 0;
}

typedef struct {
    long long d;
    int v;
} Item;

static void push(Item *h, int *n, Item x)
{
    int i = (*n)++;
    h[i] = x;
    while (i > 0 && h[(i - 1) / 2].d > h[i].d) {
        Item t = h[i];
        h[i] = h[(i - 1) / 2];
        h[(i - 1) / 2] = t;
        i = (i - 1) / 2;
    }
}

static Item pop(Item *h, int *n)
{
    Item top = h[0];
    h[0] = h[--*n];
    for (int i = 0;;) {
        int l = 2 * i + 1, r = l + 1, s = i;
        if (l < *n && h[l].d < h[s].d)
            s = l;
        if (r < *n && h[r].d < h[s].d)
            s = r;
        if (s == i)
            break;
        Item t = h[i];
        h[i] = h[s];
        h[s] = t;
        i = s;
    }
    return top;
}

void dijkstra(const WGraph *g, int s, long long *dist, int *parent)
{
    int n = g->n, m = g->start[n], hs = 0;
    Item *h = xcalloc((size_t)m + 1, sizeof *h); /* 每條邊最多 push 一次，加上起點 */
    for (int v = 0; v < n; v++) {
        dist[v] = DIST_INF;
        parent[v] = -1;
    }
    dist[s] = 0;
    push(h, &hs, (Item){0, s});
    while (hs > 0) {
        Item it = pop(h, &hs);
        int u = it.v;
        if (it.d > dist[u])
            continue; /* 過期的項目：u 已經用更短的距離處理過了 */
        /* 到這裡 dist[u] 就確定了：heap 裡其他的都 >= 它，而邊權重非負，繞路不會更短 */
        for (int i = g->start[u]; i < g->start[u + 1]; i++) {
            int v = g->to[i];
            long long nd = dist[u] + g->w[i];
            if (nd < dist[v]) { /* 鬆弛 (relax) */
                dist[v] = nd;
                parent[v] = u;
                push(h, &hs, (Item){nd, v});
            }
        }
    }
    free(h);
}

void dijkstra_dense(int n, const int *w, int s, long long *dist, int *parent)
{
    char *done = xcalloc((size_t)n, 1);
    for (int v = 0; v < n; v++) {
        dist[v] = DIST_INF;
        parent[v] = -1;
    }
    dist[s] = 0;
    for (int round = 0; round < n; round++) {
        int u = -1;
        for (int v = 0; v < n; v++) /* EXTRACT-MIN 用線性掃描 */
            if (!done[v] && dist[v] < DIST_INF && (u < 0 || dist[v] < dist[u]))
                u = v;
        if (u < 0)
            break;
        done[u] = 1;
        for (int v = 0; v < n; v++) {
            int c = w[u * n + v];
            if (c >= 0 && dist[u] + c < dist[v]) {
                dist[v] = dist[u] + c;
                parent[v] = u;
            }
        }
    }
    free(done);
}

int dij_path(const int *parent, int s, int t, int *path)
{
    int len = 0;
    for (int v = t; v != -1; v = parent[v])
        path[len++] = v;
    if (path[len - 1] != s)
        return 0;
    for (int l = 0, r = len - 1; l < r; l++, r--) {
        int x = path[l];
        path[l] = path[r];
        path[r] = x;
    }
    return len;
}
