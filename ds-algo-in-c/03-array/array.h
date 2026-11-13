#ifndef ARRAY_H
#define ARRAY_H

/* 動態陣列 (dynamic array)：容量不夠時加倍。 */
typedef struct {
    int *data;
    int size;
    int cap;
} Vec;

void vec_init(Vec *v);
void vec_free(Vec *v);
void vec_push(Vec *v, int x);            /* 尾端加入，攤銷 O(1) */
int  vec_pop(Vec *v);                    /* 尾端移除，O(1)；不可為空 */
void vec_insert(Vec *v, int idx, int x); /* 插在 idx，後面全部右移，O(n)；0 <= idx <= size */
int  vec_erase(Vec *v, int idx);         /* 刪掉 idx，後面全部左移，O(n)；回傳被刪的值 */
int  vec_find(const Vec *v, int x);      /* 線性搜尋，找不到回傳 -1 */

/* 合併兩個已排序陣列到 out（大小至少 n + m），回傳 n + m。穩定：相等時 a 先。 */
int merge_sorted(const int *a, int n, const int *b, int m, int *out);

/* 二維陣列用一維存（row-major，C 的預設）：a[r][c] 在 r * cols + c。 */
int  row_major_index(int r, int c, int cols);
void mat_transpose(const int *a, int rows, int cols, int *out);        /* out 是 cols × rows */
void mat_mul(const int *a, const int *b, int n, int m, int p, int *out); /* (n×m)(m×p) = n×p */

/* 稀疏矩陣 (sparse matrix)：只存非零元素的 (row, col, value)。 */
typedef struct {
    int r, c, v;
} Triplet;

int  sparse_from_dense(const int *a, int rows, int cols, Triplet *out); /* 回傳非零個數 */
void sparse_to_dense(const Triplet *t, int k, int rows, int cols, int *out);

#endif
