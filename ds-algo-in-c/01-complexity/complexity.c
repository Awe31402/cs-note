#include "complexity.h"

#include <stdlib.h>

long count_constant(int n)
{
    (void)n;
    long c = 0;
    for (int i = 0; i < 100; i++) /* 100 是常數，n 再大也一樣 */
        c++;
    return c;
}

long count_log(int n)
{
    long c = 0;
    for (long i = 1; i <= n; i *= 2)
        c++;
    return c;
}

long count_sqrt(int n)
{
    long c = 0;
    for (long i = 1; i * i <= n; i++)
        c++;
    return c;
}

long count_linear(int n)
{
    long c = 0;
    for (int i = 0; i < n; i++)
        c++;
    return c;
}

long count_n_log_n(int n)
{
    long c = 0;
    for (int i = 0; i < n; i++)
        for (long j = 1; j <= n; j *= 2)
            c++;
    return c;
}

long count_triangular(int n)
{
    long c = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j <= i; j++)
            c++;
    return c;
}

long count_quadratic(int n)
{
    long c = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            c++;
    return c;
}

/* 模擬動態陣列：容量不夠時開新陣列，把舊元素一個一個搬過去。
 * grow(cap) 決定新容量。 */
static long push_n(int n, int (*grow)(int))
{
    int cap = 1, size = 0;
    int *a = malloc(sizeof *a);
    long copies = 0;
    for (int x = 0; x < n; x++) {
        if (size == cap) {
            int new_cap = grow(cap);
            int *b = malloc((size_t)new_cap * sizeof *b);
            for (int i = 0; i < size; i++) {
                b[i] = a[i];
                copies++;
            }
            free(a);
            a = b;
            cap = new_cap;
        }
        a[size++] = x;
    }
    free(a);
    return copies;
}

static int grow_double(int cap) { return cap * 2; }
static int grow_plus1(int cap) { return cap + 1; }

long dynarray_copies_double(int n) { return push_n(n, grow_double); }
long dynarray_copies_plus1(int n) { return push_n(n, grow_plus1); }
