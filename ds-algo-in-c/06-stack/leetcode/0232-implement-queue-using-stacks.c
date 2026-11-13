/* LeetCode 232 · Implement Queue using Stacks
 * 思路：兩個 stack。in 負責 push；out 負責 pop/peek。
 *       out 空了才把 in 全部倒過去（倒一次順序就反過來，剛好變成 FIFO）。
 *       每個元素最多被搬一次 → 攤銷 O(1)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
#define CAP 100 /* 題目：最多 100 次操作 */

typedef struct {
    int in[CAP], out[CAP];
    int in_top, out_top;
} MyQueue;

MyQueue *myQueueCreate(void)
{
    return calloc(1, sizeof(MyQueue));
}

void myQueuePush(MyQueue *q, int x)
{
    q->in[q->in_top++] = x;
}

static void shift(MyQueue *q)
{
    if (q->out_top == 0) /* 只有 out 空了才倒，否則順序會亂 */
        while (q->in_top > 0)
            q->out[q->out_top++] = q->in[--q->in_top];
}

int myQueuePop(MyQueue *q)
{
    shift(q);
    return q->out[--q->out_top];
}

int myQueuePeek(MyQueue *q)
{
    shift(q);
    return q->out[q->out_top - 1];
}

bool myQueueEmpty(MyQueue *q)
{
    return q->in_top == 0 && q->out_top == 0;
}

void myQueueFree(MyQueue *q)
{
    free(q);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    MyQueue *q = myQueueCreate();
    myQueuePush(q, 1);
    myQueuePush(q, 2);
    assert(myQueuePeek(q) == 1);
    assert(myQueuePop(q) == 1);
    assert(!myQueueEmpty(q));
    myQueuePush(q, 3); /* out 裡還有 2，3 先留在 in */
    assert(myQueuePop(q) == 2);
    assert(myQueuePop(q) == 3);
    assert(myQueueEmpty(q));
    myQueueFree(q);

    /* 隨機對照一個普通陣列佇列 */
    srand(232);
    q = myQueueCreate();
    int model[CAP], head = 0, tail = 0, ops = 0;
    while (ops < CAP) {
        if (head == tail || rand() % 2) {
            myQueuePush(q, ops);
            model[tail++] = ops;
        } else {
            assert(myQueuePeek(q) == model[head]);
            assert(myQueuePop(q) == model[head++]);
        }
        assert(myQueueEmpty(q) == (head == tail));
        ops++;
    }
    myQueueFree(q);
    puts("0232: passed");
    return 0;
}
