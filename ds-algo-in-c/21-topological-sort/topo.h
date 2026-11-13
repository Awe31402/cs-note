#ifndef TOPO_H
#define TOPO_H

/* 有向圖，用 CSR (compressed sparse row) 存：u 的鄰居是 adj[start[u] .. start[u+1]-1]。
 * 從邊的清單一次建好，之後不再改動。 */
typedef struct {
    int n;
    int *start; /* 長度 n + 1 */
    int *adj;   /* 長度 m */
} Digraph;

void digraph_build(Digraph *g, int n, const int (*edges)[2], int m); /* edges[i] = {u, v} 代表 u → v */
void digraph_free(Digraph *g);

/* Kahn 演算法：不斷拿出入度 (in-degree) 為 0 的頂點。回傳排進 order 的頂點數；< n 代表有環。 */
int topo_kahn(const Digraph *g, int *order);

/* DFS 版（CLRS 20.4）：依完成時間由大到小排。有環回傳 0，沒環回傳 1 並填好 order。 */
int topo_dfs(const Digraph *g, int *order);

/* 找出一個環：回傳環上的頂點數 k，cycle[0..k-1] 依序是 c0 → c1 → ... → c(k-1) → c0。沒有環回傳 0。 */
int find_cycle(const Digraph *g, int *cycle);

#endif
