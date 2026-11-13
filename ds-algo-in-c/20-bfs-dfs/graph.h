#ifndef GRAPH_H
#define GRAPH_H

/* 鄰接串列 (adjacency list)：adj[u] 是 u 的所有鄰居，用動態陣列存。
 * 有向圖；無向圖就兩個方向各加一次。頂點編號 0 .. n-1。 */
typedef struct {
    int n;
    int **adj;
    int *deg; /* adj[u] 目前有幾個 */
    int *cap;
} Graph;

void graph_init(Graph *g, int n);
void graph_free(Graph *g);
void graph_add_edge(Graph *g, int u, int v);            /* u → v */
void graph_add_undirected(Graph *g, int u, int v);      /* u — v */

/* BFS（CLRS 20.2）：dist[v] = 從 s 出發的最少邊數，走不到是 -1；parent[v] 是 BFS 樹上的父節點（-1 = 沒有）。
 * 回傳走到的頂點數。 */
int bfs(const Graph *g, int s, int *dist, int *parent);

/* 由 parent 陣列還原 s → t 的路徑，寫進 path，回傳頂點數；t 走不到回傳 0。 */
int build_path(const int *parent, int s, int t, int *path);

/* DFS（CLRS 20.3）：走完所有頂點，記錄發現時間 d[v]、完成時間 f[v]（時間從 1 開始）。 */
void dfs(const Graph *g, int *d, int *f, int *parent);

/* 迴圈版 DFS：從 s 出發，依「第一次被拜訪」的順序寫進 order，回傳個數。 */
int dfs_iter(const Graph *g, int s, int *order);

/* 無向圖的連通元件 (connected components)：comp[v] = 元件編號，回傳元件數。 */
int connected_components(const Graph *g, int *comp);

#endif
