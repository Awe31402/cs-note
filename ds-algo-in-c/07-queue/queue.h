#ifndef QUEUE_H
#define QUEUE_H

/* ---- 環狀佇列 (circular queue)，固定容量 ----
 * 用 head + count 表示，不需要「浪費一格」來分辨空和滿。 */
typedef struct {
    int *data;
    int cap;
    int head;  /* 最前面元素的位置 */
    int count; /* 目前幾個 */
} CQueue;

void cq_init(CQueue *q, int cap);
void cq_free(CQueue *q);
int  cq_push(CQueue *q, int x); /* 成功 0，滿了 -1 */
int  cq_pop(CQueue *q);         /* 不可為空 */
int  cq_front(const CQueue *q);
int  cq_empty(const CQueue *q);
int  cq_full(const CQueue *q);

/* ---- 串列佇列 (linked queue) ----  尾端進、頭端出，兩個都 O(1) */
typedef struct QNode {
    int val;
    struct QNode *next;
} QNode;

typedef struct {
    QNode *head, *tail;
    int size;
} LQueue;

void lq_init(LQueue *q);
void lq_free(LQueue *q);
void lq_push(LQueue *q, int x);
int  lq_pop(LQueue *q); /* 不可為空 */

/* ---- 雙端佇列 (deque)：環狀陣列，滿了容量加倍 ----  兩端 push/pop 都是攤銷 O(1) */
typedef struct {
    int *data;
    int cap;
    int head;
    int size;
} Deque;

void dq_init(Deque *d);
void dq_free(Deque *d);
void dq_push_front(Deque *d, int x);
void dq_push_back(Deque *d, int x);
int  dq_pop_front(Deque *d); /* 不可為空 */
int  dq_pop_back(Deque *d);  /* 不可為空 */
int  dq_front(const Deque *d);
int  dq_back(const Deque *d);
int  dq_get(const Deque *d, int i); /* 第 i 個（0 = front），O(1) */

#endif
