#ifndef DSU_H
#define DSU_H

/* 並查集 (Disjoint Set Union / Union-Find)，CLRS 19.3 的樹狀森林版本。
 * 兩個優化：依秩合併 (union by rank) + 路徑壓縮 (path compression)
 * → 每個操作攤銷 O(α(n))，α 是反 Ackermann 函數，實務上 <= 4。 */
typedef struct {
    int *parent;
    int *rank; /* 樹高的上界 */
    int *size; /* 只有根的值有意義：這個集合有幾個元素 */
    int n;
    int sets;  /* 目前有幾個集合 */
} DSU;

void dsu_init(DSU *d, int n); /* MAKE-SET：每個元素自己一個集合 */
void dsu_free(DSU *d);
int  dsu_find(DSU *d, int x);         /* FIND-SET：回傳代表元素（根），順便壓縮路徑 */
int  dsu_union(DSU *d, int a, int b); /* UNION：合併成功回傳 1；本來就同一個集合回傳 0 */
int  dsu_same(DSU *d, int a, int b);
int  dsu_size(DSU *d, int x);         /* x 所在集合的大小 */

#endif
