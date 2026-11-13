#ifndef BELLMAN_FORD_H
#define BELLMAN_FORD_H

#include <limits.h>

#define BF_INF (LLONG_MAX / 4)

typedef struct {
    int u, v, w; /* u → v，權重可以是負的 */
} BEdge;

/* Bellman-Ford（CLRS 22.1）：所有邊鬆弛 n-1 輪，再多做一輪檢查。
 * 回傳 1 = 成功；回傳 0 = 從 s 走得到負環（此時最短路徑沒有定義）。O(VE)。 */
int bellman_ford(int n, const BEdge *e, int m, int s, long long *dist, int *parent);

/* 找出圖中任何一個負環（不限從哪裡走得到）。回傳環上的頂點數 k，
 * cycle[0] → cycle[1] → ... → cycle[k-1] → cycle[0]；沒有負環回傳 0。 */
int find_negative_cycle(int n, const BEdge *e, int m, int *cycle);

/* DAG 上的最短路徑（CLRS 22.2）：依拓撲順序鬆弛一次就好，O(V + E)，可以有負權重。
 * 不是 DAG 回傳 0。 */
int dag_shortest_paths(int n, const BEdge *e, int m, int s, long long *dist, int *parent);

#endif
