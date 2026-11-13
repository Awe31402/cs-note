#ifndef HEAP_H
#define HEAP_H

/* Max-heap，元素為 int，陣列從 0 開始編號。
 *   parent(i) = (i - 1) / 2
 *   left(i)   = 2 * i + 1
 *   right(i)  = 2 * i + 2
 * 書上（CLRS、Thareja）都用 1-based：parent = i/2, left = 2i, right = 2i+1。
 */
typedef struct {
    int *data;
    int size;
    int cap;
} Heap;

void heap_init(Heap *h);
void heap_free(Heap *h);

void heap_push(Heap *h, int x);              /* O(log n) */
int  heap_pop(Heap *h);                      /* O(log n)，取出最大值；heap 不可為空 */
int  heap_peek(const Heap *h);               /* O(1)，看最大值；heap 不可為空 */
void heap_build(Heap *h, const int *a, int n); /* O(n)，用 a 取代 h 的內容 */

void heap_sort(int *a, int n);               /* O(n log n)，原地、由小到大 */

#endif
