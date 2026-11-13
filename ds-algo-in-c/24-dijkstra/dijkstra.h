#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <limits.h>

#define DIST_INF (LLONG_MAX / 4) /* 走不到。除以 4：兩個 INF 相加也不會溢位 */

/* 有權重的有向圖（CSR）。Dijkstra 要求所有權重 >= 0。 */
typedef struct {
    int n;
    int *start; /* 長度 n + 1 */
    int *to, *w;
} WGraph;

void wgraph_build(WGraph *g, int n, const int (*edges)[3], int m); /* edges[i] = {u, v, w} 代表 u → v 權重 w */
void wgraph_free(WGraph *g);

/* 二元堆積版（lazy：同一個頂點可能在 heap 裡好幾次，拿出來時已經確定就跳過）。O((V + E) log E)。
 * dist[v] = 最短距離（走不到是 DIST_INF），parent[v] = 最短路徑樹上的前一個頂點（-1 = 沒有）。 */
void dijkstra(const WGraph *g, int s, long long *dist, int *parent);

/* 陣列版：w 是 n×n 矩陣，w[u*n+v] < 0 代表沒有邊。O(V²)，適合稠密圖。 */
void dijkstra_dense(int n, const int *w, int s, long long *dist, int *parent);

/* 由 parent 還原 s → t 的路徑，回傳頂點數；走不到回傳 0。 */
int dij_path(const int *parent, int s, int t, int *path);

#endif
