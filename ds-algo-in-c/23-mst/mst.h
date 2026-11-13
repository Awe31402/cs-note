#ifndef MST_H
#define MST_H

/* 最小生成樹 (Minimum Spanning Tree)：連通無向圖中，連起所有頂點、總權重最小的樹（剛好 n-1 條邊）。 */
typedef struct {
    int u, v, w;
} Edge;

/* Kruskal：邊由小到大排序，不會形成環就選（用 Union-Find 判斷）。會把 edges 排序。
 * 選中的邊寫進 chosen（至少 n-1 格），*k = 選了幾條。回傳總權重；圖不連通回傳 -1。O(E log E)。 */
long long kruskal(int n, Edge *edges, int m, Edge *chosen, int *k);

/* Prim（二元堆積版）：從頂點 0 長出一棵樹，每次加入「連到樹外、權重最小」的邊。O(E log E)。 */
long long prim_heap(int n, const Edge *edges, int m, Edge *chosen, int *k);

/* Prim（陣列版）：w 是 n×n 鄰接矩陣，w[i*n+j] < 0 代表沒有邊。O(V²)，適合稠密圖。
 * parent[v] = v 在樹上的父節點（根是 -1）。回傳總權重；不連通回傳 -1。 */
long long prim_dense(int n, const int *w, int *parent);

#endif
