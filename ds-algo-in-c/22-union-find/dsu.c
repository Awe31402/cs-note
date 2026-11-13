#include "dsu.h"

#include <stdlib.h>

void dsu_init(DSU *d, int n)
{
    d->parent = malloc((size_t)(n ? n : 1) * sizeof *d->parent);
    d->rank = calloc((size_t)(n ? n : 1), sizeof *d->rank);
    d->size = malloc((size_t)(n ? n : 1) * sizeof *d->size);
    if (!d->parent || !d->rank || !d->size)
        abort();
    for (int i = 0; i < n; i++) {
        d->parent[i] = i; /* 自己是自己的根 */
        d->size[i] = 1;
    }
    d->n = n;
    d->sets = n;
}

void dsu_free(DSU *d)
{
    free(d->parent);
    free(d->rank);
    free(d->size);
    d->n = d->sets = 0;
}

int dsu_find(DSU *d, int x)
{
    int root = x;
    while (d->parent[root] != root) /* 第一趟：找到根 */
        root = d->parent[root];
    while (d->parent[x] != root) { /* 第二趟：路上每個節點都直接指向根 */
        int next = d->parent[x];
        d->parent[x] = root;
        x = next;
    }
    return root; /* 用迴圈不用遞迴：還沒壓縮過的長鏈不會爆堆疊 */
}

int dsu_union(DSU *d, int a, int b)
{
    a = dsu_find(d, a);
    b = dsu_find(d, b);
    if (a == b)
        return 0;
    if (d->rank[a] < d->rank[b]) { /* 矮的樹接到高的樹下面，高度才不會長 */
        int t = a;
        a = b;
        b = t;
    }
    d->parent[b] = a;
    d->size[a] += d->size[b];
    if (d->rank[a] == d->rank[b]) /* 一樣高時，合併後才會多一層 */
        d->rank[a]++;
    d->sets--;
    return 1;
}

int dsu_same(DSU *d, int a, int b)
{
    return dsu_find(d, a) == dsu_find(d, b);
}

int dsu_size(DSU *d, int x)
{
    return d->size[dsu_find(d, x)];
}
