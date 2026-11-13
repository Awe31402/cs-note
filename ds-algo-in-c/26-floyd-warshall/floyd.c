#include "floyd.h"

void floyd_warshall(int n, long long *d, int *next)
{
    if (next)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                next[i * n + j] = d[i * n + j] < FW_INF ? j : -1;
    /* 第 k 輪結束後：d[i][j] = 「中間只經過頂點 0..k」的最短距離。
     * k 一定要放在最外層！ */
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++) {
            if (d[i * n + k] == FW_INF)
                continue; /* i 到不了 k：經過 k 不可能更短 */
            for (int j = 0; j < n; j++) {
                if (d[k * n + j] == FW_INF)
                    continue;
                long long via = d[i * n + k] + d[k * n + j];
                if (via < d[i * n + j]) {
                    d[i * n + j] = via;
                    if (next)
                        next[i * n + j] = next[i * n + k]; /* 先往 k 的方向走 */
                }
            }
        }
}

int fw_has_negative_cycle(int n, const long long *d)
{
    for (int i = 0; i < n; i++)
        if (d[i * n + i] < 0) /* 從 i 繞一圈回來比 0 還小 */
            return 1;
    return 0;
}

int fw_path(int n, const int *next, int u, int v, int *path)
{
    if (next[u * n + v] == -1)
        return u == v ? (path[0] = u, 1) : 0;
    int len = 0;
    path[len++] = u;
    while (u != v) {
        u = next[u * n + v];
        path[len++] = u;
    }
    return len;
}

void transitive_closure(int n, unsigned char *r)
{
    for (int i = 0; i < n; i++)
        r[i * n + i] = 1; /* 自己走得到自己 */
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++)
            if (r[i * n + k])
                for (int j = 0; j < n; j++)
                    r[i * n + j] |= r[k * n + j]; /* 走得到 k，又從 k 走得到 j */
}
