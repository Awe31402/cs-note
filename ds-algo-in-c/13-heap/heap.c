#include "heap.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

/* 新值放在 i，一路跟父節點比，比父節點大就往上換。 */
static void sift_up(int *a, int i)
{
    while (i > 0) {
        int p = (i - 1) / 2;
        if (a[p] >= a[i])
            break;
        swap(&a[p], &a[i]);
        i = p;
    }
}

/* a[i] 可能比子節點小，跟「較大的那個子節點」交換，一路往下。
 * 對應 CLRS 的 MAX-HEAPIFY。 */
static void sift_down(int *a, int n, int i)
{
    for (;;) {
        int l = 2 * i + 1;
        int r = l + 1;
        int largest = i;
        if (l < n && a[l] > a[largest])
            largest = l;
        if (r < n && a[r] > a[largest])
            largest = r;
        if (largest == i)
            return;
        swap(&a[i], &a[largest]);
        i = largest;
    }
}

static void reserve(Heap *h, int need)
{
    if (need <= h->cap)
        return;
    int cap = h->cap ? h->cap : 8;
    while (cap < need)
        cap *= 2;
    int *p = realloc(h->data, (size_t)cap * sizeof *p);
    if (!p)
        abort();
    h->data = p;
    h->cap = cap;
}

void heap_init(Heap *h)
{
    h->data = NULL;
    h->size = 0;
    h->cap = 0;
}

void heap_free(Heap *h)
{
    free(h->data);
    heap_init(h);
}

void heap_push(Heap *h, int x)
{
    reserve(h, h->size + 1);
    h->data[h->size] = x;
    sift_up(h->data, h->size);
    h->size++;
}

int heap_pop(Heap *h)
{
    assert(h->size > 0);
    int top = h->data[0];
    h->size--;
    h->data[0] = h->data[h->size]; /* 最後一個搬到根，再往下沉 */
    sift_down(h->data, h->size, 0);
    return top;
}

int heap_peek(const Heap *h)
{
    assert(h->size > 0);
    return h->data[0];
}

/* 從最後一個非葉節點 (n/2 - 1) 往回做 sift_down。
 * 對應 CLRS 的 BUILD-MAX-HEAP，總成本 O(n)。 */
void heap_build(Heap *h, const int *a, int n)
{
    reserve(h, n);
    if (n > 0)
        memcpy(h->data, a, (size_t)n * sizeof *a);
    h->size = n;
    for (int i = n / 2 - 1; i >= 0; i--)
        sift_down(h->data, n, i);
}

/* 先建 max-heap，再反覆把最大值（根）換到尾端，縮小 heap。 */
void heap_sort(int *a, int n)
{
    for (int i = n / 2 - 1; i >= 0; i--)
        sift_down(a, n, i);
    for (int end = n - 1; end > 0; end--) {
        swap(&a[0], &a[end]);
        sift_down(a, end, 0);
    }
}
