#include "array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void reserve(Vec *v, int need)
{
    if (need <= v->cap)
        return;
    int cap = v->cap ? v->cap : 8;
    while (cap < need)
        cap *= 2;
    int *p = realloc(v->data, (size_t)cap * sizeof *p);
    if (!p)
        abort();
    v->data = p;
    v->cap = cap;
}

void vec_init(Vec *v)
{
    v->data = NULL;
    v->size = 0;
    v->cap = 0;
}

void vec_free(Vec *v)
{
    free(v->data);
    vec_init(v);
}

void vec_push(Vec *v, int x)
{
    reserve(v, v->size + 1);
    v->data[v->size++] = x;
}

int vec_pop(Vec *v)
{
    assert(v->size > 0);
    return v->data[--v->size];
}

void vec_insert(Vec *v, int idx, int x)
{
    assert(idx >= 0 && idx <= v->size);
    reserve(v, v->size + 1);
    /* 從後面往前搬，才不會蓋掉還沒搬的元素（Thareja 3.5.2） */
    memmove(&v->data[idx + 1], &v->data[idx], (size_t)(v->size - idx) * sizeof *v->data);
    v->data[idx] = x;
    v->size++;
}

int vec_erase(Vec *v, int idx)
{
    assert(idx >= 0 && idx < v->size);
    int x = v->data[idx];
    memmove(&v->data[idx], &v->data[idx + 1], (size_t)(v->size - idx - 1) * sizeof *v->data);
    v->size--;
    return x;
}

int vec_find(const Vec *v, int x)
{
    for (int i = 0; i < v->size; i++)
        if (v->data[i] == x)
            return i;
    return -1;
}

int merge_sorted(const int *a, int n, const int *b, int m, int *out)
{
    int i = 0, j = 0, k = 0;
    while (i < n && j < m)
        out[k++] = a[i] <= b[j] ? a[i++] : b[j++];
    while (i < n)
        out[k++] = a[i++];
    while (j < m)
        out[k++] = b[j++];
    return k;
}

int row_major_index(int r, int c, int cols)
{
    return r * cols + c;
}

void mat_transpose(const int *a, int rows, int cols, int *out)
{
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            out[row_major_index(c, r, rows)] = a[row_major_index(r, c, cols)];
}

void mat_mul(const int *a, const int *b, int n, int m, int p, int *out)
{
    for (int i = 0; i < n; i++)
        for (int j = 0; j < p; j++) {
            int s = 0;
            for (int k = 0; k < m; k++)
                s += a[i * m + k] * b[k * p + j];
            out[i * p + j] = s;
        }
}

int sparse_from_dense(const int *a, int rows, int cols, Triplet *out)
{
    int k = 0;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            if (a[r * cols + c] != 0)
                out[k++] = (Triplet){r, c, a[r * cols + c]};
    return k;
}

void sparse_to_dense(const Triplet *t, int k, int rows, int cols, int *out)
{
    memset(out, 0, (size_t)rows * (size_t)cols * sizeof *out);
    for (int i = 0; i < k; i++)
        out[t[i].r * cols + t[i].c] = t[i].v;
}
