#include "queue.h"

#include <assert.h>
#include <stdlib.h>

/* ---- 環狀佇列 ---- */

void cq_init(CQueue *q, int cap)
{
    q->data = malloc((size_t)cap * sizeof *q->data);
    if (!q->data)
        abort();
    q->cap = cap;
    q->head = 0;
    q->count = 0;
}

void cq_free(CQueue *q)
{
    free(q->data);
    q->data = NULL;
}

int cq_push(CQueue *q, int x)
{
    if (q->count == q->cap)
        return -1;
    q->data[(q->head + q->count) % q->cap] = x; /* 尾巴 = head + count，超過就繞回 0 */
    q->count++;
    return 0;
}

int cq_pop(CQueue *q)
{
    assert(q->count > 0);
    int x = q->data[q->head];
    q->head = (q->head + 1) % q->cap;
    q->count--;
    return x;
}

int cq_front(const CQueue *q)
{
    assert(q->count > 0);
    return q->data[q->head];
}

int cq_empty(const CQueue *q) { return q->count == 0; }
int cq_full(const CQueue *q) { return q->count == q->cap; }

/* ---- 串列佇列 ---- */

void lq_init(LQueue *q)
{
    q->head = q->tail = NULL;
    q->size = 0;
}

void lq_push(LQueue *q, int x)
{
    QNode *n = malloc(sizeof *n);
    if (!n)
        abort();
    n->val = x;
    n->next = NULL;
    if (q->tail)
        q->tail->next = n;
    else
        q->head = n; /* 原本是空的：head 也要指向它 */
    q->tail = n;
    q->size++;
}

int lq_pop(LQueue *q)
{
    assert(q->head);
    QNode *n = q->head;
    int x = n->val;
    q->head = n->next;
    if (!q->head)
        q->tail = NULL; /* 變空了：tail 也要清掉，否則是懸空指標 */
    free(n);
    q->size--;
    return x;
}

void lq_free(LQueue *q)
{
    while (q->head)
        lq_pop(q);
}

/* ---- 雙端佇列 ---- */

void dq_init(Deque *d)
{
    d->data = NULL;
    d->cap = 0;
    d->head = 0;
    d->size = 0;
}

void dq_free(Deque *d)
{
    free(d->data);
    dq_init(d);
}

/* 擴容時不能直接 realloc：環狀的資料可能繞過尾端，要照邏輯順序搬到新陣列的開頭 */
static void grow(Deque *d)
{
    int cap = d->cap ? d->cap * 2 : 8;
    int *p = malloc((size_t)cap * sizeof *p);
    if (!p)
        abort();
    for (int i = 0; i < d->size; i++)
        p[i] = d->data[(d->head + i) % d->cap];
    free(d->data);
    d->data = p;
    d->cap = cap;
    d->head = 0;
}

void dq_push_front(Deque *d, int x)
{
    if (d->size == d->cap)
        grow(d);
    d->head = (d->head - 1 + d->cap) % d->cap; /* 往左一格；+cap 避免負數取餘數 */
    d->data[d->head] = x;
    d->size++;
}

void dq_push_back(Deque *d, int x)
{
    if (d->size == d->cap)
        grow(d);
    d->data[(d->head + d->size) % d->cap] = x;
    d->size++;
}

int dq_pop_front(Deque *d)
{
    assert(d->size > 0);
    int x = d->data[d->head];
    d->head = (d->head + 1) % d->cap;
    d->size--;
    return x;
}

int dq_pop_back(Deque *d)
{
    assert(d->size > 0);
    d->size--;
    return d->data[(d->head + d->size) % d->cap];
}

int dq_get(const Deque *d, int i)
{
    assert(i >= 0 && i < d->size);
    return d->data[(d->head + i) % d->cap];
}

int dq_front(const Deque *d) { return dq_get(d, 0); }
int dq_back(const Deque *d) { return dq_get(d, d->size - 1); }
