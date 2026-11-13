/* LeetCode 225 · Implement Stack using Queues
 * 思路：只用一個 queue。push x 之後，把 x 前面的 size-1 個元素依序「出隊再入隊」，
 *       x 就轉到最前面了。push O(n)，pop/top O(1)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
#define CAP 101 /* 題目：最多 100 次操作；環狀 queue */

typedef struct {
    int q[CAP];
    int head, size;
} MyStack;

static void enq(MyStack *s, int x)
{
    s->q[(s->head + s->size) % CAP] = x;
    s->size++;
}

static int deq(MyStack *s)
{
    int x = s->q[s->head];
    s->head = (s->head + 1) % CAP;
    s->size--;
    return x;
}

MyStack *myStackCreate(void)
{
    return calloc(1, sizeof(MyStack));
}

void myStackPush(MyStack *s, int x)
{
    enq(s, x);
    for (int i = 0; i < s->size - 1; i++) /* 把舊的轉到 x 後面 */
        enq(s, deq(s));
}

int myStackPop(MyStack *s)
{
    return deq(s);
}

int myStackTop(MyStack *s)
{
    return s->q[s->head];
}

bool myStackEmpty(MyStack *s)
{
    return s->size == 0;
}

void myStackFree(MyStack *s)
{
    free(s);
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    MyStack *s = myStackCreate();
    myStackPush(s, 1);
    myStackPush(s, 2);
    assert(myStackTop(s) == 2);
    assert(myStackPop(s) == 2);
    assert(!myStackEmpty(s));
    myStackPush(s, 3);
    assert(myStackPop(s) == 3 && myStackPop(s) == 1 && myStackEmpty(s));
    myStackFree(s);

    srand(225);
    s = myStackCreate();
    int model[100], n = 0;
    for (int k = 0; k < 100; k++) {
        if (n == 0 || rand() % 2) {
            myStackPush(s, k);
            model[n++] = k;
        } else {
            assert(myStackTop(s) == model[n - 1]);
            assert(myStackPop(s) == model[--n]);
        }
        assert(myStackEmpty(s) == (n == 0));
    }
    myStackFree(s);
    puts("0225: passed");
    return 0;
}
