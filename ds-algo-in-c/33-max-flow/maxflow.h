#ifndef MAXFLOW_H
#define MAXFLOW_H

/* 最大流 (maximum flow)：有向圖，每條邊有容量 (capacity)。從源點 s 送到匯點 t，最多能送多少？
 * 容量用 n×n 矩陣存：cap[u*n+v] = u → v 的容量（0 = 沒有邊）。 */

/* Edmonds-Karp（CLRS 24.2）= Ford-Fulkerson 方法 + 每次用 BFS 找「邊數最少」的增廣路徑。O(V E²)。
 * cap 會被改成剩餘網路 (residual network) 的容量；flow[u*n+v] 寫入最後每條邊的流量（可為 NULL）。 */
long long edmonds_karp(int n, long long *cap, int s, int t, long long *flow);

/* 做完最大流之後，剩餘網路中從 s 走得到的頂點 = 最小割 (min cut) 的 s 那一側。side[v] = 1 代表在 s 側。 */
void min_cut_side(int n, const long long *residual, int s, char *side);

/* 二分圖最大匹配 (maximum bipartite matching, CLRS 24.3 / 25.1)：左邊 nl 個、右邊 nr 個，
 * edges[i] = {左, 右}。match_l[u] = 左邊 u 配到的右邊頂點（-1 = 沒配到）。回傳配對數。
 * 用增廣路徑（Kuhn 演算法），O(V E)。 */
int bipartite_matching(int nl, int nr, const int (*edges)[2], int m, int *match_l);

#endif
