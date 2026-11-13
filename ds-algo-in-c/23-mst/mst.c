#include "mst.h"

#include <limits.h>
#include <stdlib.h>

static void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p)
        abort();
    return p;
}

/* ---- Kruskal ---- */

static int find(int *p, int x)
{
    while (p[x] != x) {
        p[x] = p[p[x]];
        x = p[x];
    }
    return x;
}

static int cmp_edge(const void *a, const void *b)
{
    int x = ((const Edge *)a)->w, y = ((const Edge *)b)->w;
    return (x > y) - (x < y);
}

long long kruskal(int n, Edge *edges, int m, Edge *chosen, int *k)
{
    qsort(edges, (size_t)m, sizeof *edges, cmp_edge);
    int *p = xmalloc((size_t)n * sizeof *p);
    for (int i = 0; i < n; i++)
        p[i] = i;
    long long total = 0;
    *k = 0;
    for (int i = 0; i < m && *k < n - 1; i++) {
        int a = find(p, edges[i].u), b = find(p, edges[i].v);
        if (a == b)
            continue; /* 兩端已經連通：選了會形成環 */
        p[a] = b;
        chosen[(*k)++] = edges[i];
        total += edges[i].w;
    }
    free(p);
    return *k == n - 1 ? total : -1;
}

/* ---- Prim（heap 版）---- */

typedef struct {
    int w, to, from;
} Item;

static void heap_push(Item *h, int *size, Item x)
{
    int i = (*size)++;
    h[i] = x;
    while (i > 0 && h[(i - 1) / 2].w > h[i].w) {
        Item t = h[i];
        h[i] = h[(i - 1) / 2];
        h[(i - 1) / 2] = t;
        i = (i - 1) / 2;
    }
}

static Item heap_pop(Item *h, int *size)
{
    Item top = h[0];
    h[0] = h[--*size];
    for (int i = 0;;) {
        int l = 2 * i + 1, r = l + 1, s = i;
        if (l < *size && h[l].w < h[s].w)
            s = l;
        if (r < *size && h[r].w < h[s].w)
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

long long prim_heap(int n, const Edge *edges, int m, Edge *chosen, int *k)
{
    /* 建 CSR 鄰接串列（無向：兩個方向） */
    int *start = calloc((size_t)n + 1, sizeof *start), *fill = calloc((size_t)n, sizeof *fill);
    int *to = xmalloc((size_t)2 * m * sizeof *to), *wt = xmalloc((size_t)2 * m * sizeof *wt);
    if (!start || !fill)
        abort();
    for (int i = 0; i < m; i++) {
        start[edges[i].u + 1]++;
        start[edges[i].v + 1]++;
    }
    for (int v = 0; v < n; v++)
        start[v + 1] += start[v];
    for (int i = 0; i < m; i++) {
        int a = edges[i].u, b = edges[i].v;
        to[start[a] + fill[a]] = b;
        wt[start[a] + fill[a]++] = edges[i].w;
        to[start[b] + fill[b]] = a;
        wt[start[b] + fill[b]++] = edges[i].w;
    }

    char *in_tree = calloc((size_t)n, 1);
    Item *h = xmalloc((size_t)(2 * m + 1) * sizeof *h);
    int hs = 0;
    long long total = 0;
    *k = 0;
    /* lazy 版：同一個頂點可能在 heap 裡好幾次，拿出來時已經在樹裡就跳過 */
    heap_push(h, &hs, (Item){0, 0, -1});
    while (hs > 0) {
        Item it = heap_pop(h, &hs);
        if (in_tree[it.to])
            continue;
        in_tree[it.to] = 1;
        if (it.from >= 0) {
            chosen[(*k)++] = (Edge){it.from, it.to, it.w};
            total += it.w;
        }
        for (int i = start[it.to]; i < start[it.to + 1]; i++)
            if (!in_tree[to[i]])
                heap_push(h, &hs, (Item){wt[i], to[i], it.to});
    }
    free(start);
    free(fill);
    free(to);
    free(wt);
    free(in_tree);
    free(h);
    return *k == n - 1 ? total : -1;
}

/* ---- Prim（陣列版，CLRS/Thareja 的經典寫法）---- */

long long prim_dense(int n, const int *w, int *parent)
{
    int *key = xmalloc((size_t)n * sizeof *key); /* key[v] = v 連到目前這棵樹的最小邊權重 */
    char *in_tree = calloc((size_t)n, 1);
    for (int v = 0; v < n; v++) {
        key[v] = INT_MAX;
        parent[v] = -1;
    }
    key[0] = 0;
    long long total = 0;
    int added = 0;
    for (int round = 0; round < n; round++) {
        int u = -1;
        for (int v = 0; v < n; v++) /* 線性找最小的 key，O(V) */
            if (!in_tree[v] && key[v] != INT_MAX && (u < 0 || key[v] < key[u]))
                u = v;
        if (u < 0)
            break; /* 剩下的頂點都連不到：不連通 */
        in_tree[u] = 1;
        total += key[u];
        added++;
        for (int v = 0; v < n; v++) { /* 用 u 更新其他頂點的 key */
            int c = w[u * n + v];
            if (c >= 0 && !in_tree[v] && c < key[v]) {
                key[v] = c;
                parent[v] = u;
            }
        }
    }
    free(key);
    free(in_tree);
    return added == n ? total : -1;
}
