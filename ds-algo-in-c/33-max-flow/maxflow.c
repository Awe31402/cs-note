#include "maxflow.h"

#include <stdlib.h>
#include <string.h>

long long edmonds_karp(int n, long long *cap, int s, int t, long long *flow)
{
    long long *orig = NULL;
    if (flow) {
        orig = malloc((size_t)n * n * sizeof *orig);
        if (!orig)
            abort();
        memcpy(orig, cap, (size_t)n * n * sizeof *orig);
    }
    int *parent = malloc((size_t)n * sizeof *parent), *q = malloc((size_t)n * sizeof *q);
    if (!parent || !q)
        abort();
    long long total = 0;
    for (;;) {
        /* BFS：在剩餘網路找一條 s → t 的路（只走剩餘容量 > 0 的邊） */
        for (int v = 0; v < n; v++)
            parent[v] = -1;
        parent[s] = s;
        int head = 0, tail = 0;
        q[tail++] = s;
        while (head < tail && parent[t] == -1) {
            int u = q[head++];
            for (int v = 0; v < n; v++)
                if (parent[v] == -1 && cap[u * n + v] > 0) {
                    parent[v] = u;
                    q[tail++] = v;
                }
        }
        if (parent[t] == -1)
            break; /* 找不到增廣路徑 → 已經是最大流 */
        long long bottleneck = -1; /* 路上最小的剩餘容量 */
        for (int v = t; v != s; v = parent[v]) {
            long long c = cap[parent[v] * n + v];
            if (bottleneck < 0 || c < bottleneck)
                bottleneck = c;
        }
        for (int v = t; v != s; v = parent[v]) {
            int u = parent[v];
            cap[u * n + v] -= bottleneck; /* 正向用掉 */
            cap[v * n + u] += bottleneck; /* 反向邊：之後可以「退回」這些流量 */
        }
        total += bottleneck;
    }
    if (flow) { /* 流量 = 原本容量 − 剩餘容量（取正的部分） */
        for (int i = 0; i < n * n; i++) {
            long long f = orig[i] - cap[i];
            flow[i] = f > 0 ? f : 0;
        }
        free(orig);
    }
    free(parent);
    free(q);
    return total;
}

void min_cut_side(int n, const long long *residual, int s, char *side)
{
    int *q = malloc((size_t)n * sizeof *q), head = 0, tail = 0;
    if (!q)
        abort();
    memset(side, 0, (size_t)n);
    side[s] = 1;
    q[tail++] = s;
    while (head < tail) {
        int u = q[head++];
        for (int v = 0; v < n; v++)
            if (!side[v] && residual[u * n + v] > 0) {
                side[v] = 1;
                q[tail++] = v;
            }
    }
    free(q);
}

/* ---- 二分圖匹配 ---- */

typedef struct {
    int nl, nr;
    int *start, *adj; /* 左邊每個頂點的鄰居（右邊），CSR */
    int *match_r;     /* 右邊 v 配到的左邊頂點 */
    char *seen;
} BM;

/* 從左邊 u 出發找增廣路徑：u 想要 v；v 沒人要 → 直接配；v 已經有人 → 叫那個人去換別的 */
static int augment(BM *b, int u)
{
    for (int i = b->start[u]; i < b->start[u + 1]; i++) {
        int v = b->adj[i];
        if (b->seen[v])
            continue;
        b->seen[v] = 1;
        if (b->match_r[v] < 0 || augment(b, b->match_r[v])) {
            b->match_r[v] = u;
            return 1;
        }
    }
    return 0;
}

int bipartite_matching(int nl, int nr, const int (*edges)[2], int m, int *match_l)
{
    BM b = {nl, nr, calloc((size_t)nl + 1, sizeof(int)), malloc((size_t)(m ? m : 1) * sizeof(int)),
            malloc((size_t)(nr ? nr : 1) * sizeof(int)), malloc((size_t)(nr ? nr : 1))};
    int *fill = calloc((size_t)(nl ? nl : 1), sizeof *fill);
    if (!b.start || !b.adj || !b.match_r || !b.seen || !fill)
        abort();
    for (int i = 0; i < m; i++)
        b.start[edges[i][0] + 1]++;
    for (int u = 0; u < nl; u++)
        b.start[u + 1] += b.start[u];
    for (int i = 0; i < m; i++)
        b.adj[b.start[edges[i][0]] + fill[edges[i][0]]++] = edges[i][1];
    for (int v = 0; v < nr; v++)
        b.match_r[v] = -1;
    int result = 0;
    for (int u = 0; u < nl; u++) {
        memset(b.seen, 0, (size_t)nr);
        result += augment(&b, u);
    }
    for (int u = 0; u < nl; u++)
        match_l[u] = -1;
    for (int v = 0; v < nr; v++)
        if (b.match_r[v] >= 0)
            match_l[b.match_r[v]] = v;
    free(b.start);
    free(b.adj);
    free(b.match_r);
    free(b.seen);
    free(fill);
    return result;
}
