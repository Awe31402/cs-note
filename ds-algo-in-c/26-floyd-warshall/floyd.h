#ifndef FLOYD_H
#define FLOYD_H

#include <limits.h>

#define FW_INF (LLONG_MAX / 4)

/* Floyd-Warshall（CLRS 23.2）：所有點對的最短路徑，O(V³)，可以有負權重。
 * d 是 n×n 矩陣（d[i*n+j]），呼叫前放好：d[i][i] = 0、有邊放權重、沒邊放 FW_INF。呼叫後就是最短距離。
 * next 可以是 NULL；不是的話，next[i*n+j] = 從 i 往 j 的最短路徑上，i 的下一個頂點（-1 = 走不到）。 */
void floyd_warshall(int n, long long *d, int *next);

/* 做完 floyd_warshall 之後：有頂點 d[i][i] < 0 → 有負環 */
int fw_has_negative_cycle(int n, const long long *d);

/* 用 next 還原 u → v 的路徑，回傳頂點數；走不到回傳 0 */
int fw_path(int n, const int *next, int u, int v, int *path);

/* Warshall 遞移閉包 (transitive closure)：r[i*n+j] = 1 代表 i 走得到 j。呼叫前 r 放鄰接矩陣。 */
void transitive_closure(int n, unsigned char *r);

#endif
