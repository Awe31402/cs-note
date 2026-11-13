/* LeetCode 622 · Design Circular Queue
 * 思路：陣列 + head + count。尾巴位置 = (head + count) % k。
 *       用 count 分辨空（0）和滿（k），不用浪費一格。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
typedef struct {
    int *a;
    int k, head, count;
} MyCircularQueue;

MyCircularQueue *myCircularQueueCreate(int k)
{
    MyCircularQueue *q = malloc(sizeof *q);
    q->a = malloc((size_t)k * sizeof *q->a);
    q->k = k;
    q->head = 0;
    q->count = 0;
    return q;
}

bool myCircularQueueEnQueue(MyCircularQueue *q, int value)
{
    if (q->count == q->k)
        return false;
    q->a[(q->head + q->count) % q->k] = value;
    q->count++;
    return true;
}

bool myCircularQueueDeQueue(MyCircularQueue *q)
{
    if (q->count == 0)
        return false;
    q->head = (q->head + 1) % q->k;
    q->count--;
    return true;
}

int myCircularQueueFront(MyCircularQueue *q)
{
    return q->count ? q->a[q->head] : -1;
}

int myCircularQueueRear(MyCircularQueue *q)
{
    return q->count ? q->a[(q->head + q->count - 1) % q->k] : -1;
}

bool myCircularQueueIsEmpty(MyCircularQueue *q)
{
    return q->count == 0;
}

bool myCircularQueueIsFull(MyCircularQueue *q)
{
    return q->count == q->k;
}

void myCircularQueueFree(MyCircularQueue *q)
{
    free(q->a);
    free(q);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    MyCircularQueue *q = myCircularQueueCreate(3);
    assert(myCircularQueueEnQueue(q, 1));
    assert(myCircularQueueEnQueue(q, 2));
    assert(myCircularQueueEnQueue(q, 3));
    assert(!myCircularQueueEnQueue(q, 4));
    assert(myCircularQueueRear(q) == 3);
    assert(myCircularQueueIsFull(q));
    assert(myCircularQueueDeQueue(q));
    assert(myCircularQueueEnQueue(q, 4));
    assert(myCircularQueueRear(q) == 4);
    assert(myCircularQueueFront(q) == 2);
    assert(myCircularQueueDeQueue(q) && myCircularQueueDeQueue(q) && myCircularQueueDeQueue(q));
    assert(!myCircularQueueDeQueue(q));
    assert(myCircularQueueFront(q) == -1 && myCircularQueueRear(q) == -1);
    assert(myCircularQueueIsEmpty(q));
    myCircularQueueFree(q);
    puts("0622: passed");
    return 0;
}
