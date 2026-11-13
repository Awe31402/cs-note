#include "heap.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static void test_empty(void)
{
    Heap h;
    heap_init(&h);
    assert(h.size == 0);
    heap_free(&h);
}

/* Thareja Example 12.2：依序插入，根永遠是最大值。 */
static void test_push_pop(void)
{
    int in[] = {45, 36, 54, 27, 63, 72, 61, 18};
    int want[] = {72, 63, 61, 54, 45, 36, 27, 18};
    Heap h;
    heap_init(&h);
    for (int i = 0; i < LEN(in); i++)
        heap_push(&h, in[i]);
    assert(heap_peek(&h) == 72);
    for (int i = 0; i < LEN(want); i++)
        assert(heap_pop(&h) == want[i]);
    assert(h.size == 0);
    heap_free(&h);
}

/* CLRS Figure 6.3：BUILD-MAX-HEAP 的輸入與結果。 */
static void test_build_clrs(void)
{
    int in[] = {4, 1, 3, 2, 16, 9, 10, 14, 8, 7};
    int want[] = {16, 14, 10, 8, 7, 9, 3, 2, 4, 1};
    Heap h;
    heap_init(&h);
    heap_build(&h, in, LEN(in));
    assert(h.size == LEN(want));
    assert(memcmp(h.data, want, sizeof want) == 0);
    heap_free(&h);
}

static void test_duplicates_negatives(void)
{
    int in[] = {3, -1, 3, 0, -1, 3};
    int want[] = {3, 3, 3, 0, -1, -1};
    Heap h;
    heap_init(&h);
    for (int i = 0; i < LEN(in); i++)
        heap_push(&h, in[i]);
    for (int i = 0; i < LEN(want); i++)
        assert(heap_pop(&h) == want[i]);
    heap_free(&h);
}

/* 隨機測 heap_sort，答案用 qsort 對照。n = 0 和 1 也包含在內。 */
static void test_heap_sort_random(void)
{
    srand(12345);
    for (int n = 0; n <= 200; n++) {
        int *a = malloc((size_t)(n ? n : 1) * sizeof *a);
        int *b = malloc((size_t)(n ? n : 1) * sizeof *b);
        assert(a && b);
        for (int i = 0; i < n; i++)
            a[i] = b[i] = rand() % 100 - 50;
        heap_sort(a, n);
        qsort(b, (size_t)n, sizeof *b, cmp_int);
        assert(n == 0 || memcmp(a, b, (size_t)n * sizeof *a) == 0);
        free(a);
        free(b);
    }
}

int main(void)
{
    test_empty();
    test_push_pop();
    test_build_clrs();
    test_duplicates_negatives();
    test_heap_sort_random();
    puts("heap: all tests passed");
    return 0;
}
